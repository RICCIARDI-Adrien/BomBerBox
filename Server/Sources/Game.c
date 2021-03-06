/** @file Game.c
 * @see Game.h for description.
 * @author Adrien RICCIARDI
 */

#include <Configuration.h>
#include <errno.h>
#include <Game.h>
#include <Map.h>
#include <Network.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** All players. */
static TGamePlayer Game_Players[CONFIGURATION_MAXIMUM_PLAYERS_COUNT];
/** How many players in the game. */
static int Game_Players_Count;
/** How many alive players in the current game. */
static int Game_Alive_Players_Count;
/** How many connected players on the server. */
static int Game_Connected_Players_Count;

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
/** Wait for all clients to connect. */
// TODO handle player disconnection
static inline void GameWaitForPlayersConnection(void)
{
	int i, Is_Player_Ready[CONFIGURATION_MAXIMUM_PLAYERS_COUNT] = {0};
	TNetworkEvent Event;
	struct timespec Waiting_Time =
	{
		.tv_sec = 0,
		.tv_nsec = CONFIGURATION_GAME_TICK
	};
	
	// Reset players data
	Game_Players_Count = 0;
	for (i = 0; i < CONFIGURATION_MAXIMUM_PLAYERS_COUNT; i++) memset(Game_Players[i].String_Name, 0, sizeof(Game_Players[i].String_Name)); // Clear name buffers
	
	while (1)
	{
		// Check for a new player connection if there remain free player slots
		if (Game_Players_Count < CONFIGURATION_MAXIMUM_PLAYERS_COUNT)
		{
			// Did a new player attempted connection ?
			if (NetworkIsPlayerConnected(&Game_Players[Game_Players_Count].Socket, Game_Players[Game_Players_Count].String_Name))
			{
				NetworkSendCommandDrawText(&Game_Players[Game_Players_Count], "Hit Space when all players are ready.");
				printf("Client #%d connected, name : %s.\n", Game_Players_Count + 1, Game_Players[Game_Players_Count].String_Name);
				Game_Players_Count++;
			}
		}
		
		// Check if a player hit the ready key
		for (i = 0; i < Game_Players_Count; i++)
		{
			if (NetworkGetEvent(&Game_Players[i], &Event) != 0) printf("[%s:%d] Error : failed to get player #%d event (%s).\n", __FUNCTION__, __LINE__, i + 1, strerror(errno));
			else if (Event == NETWORK_EVENT_DROP_BOMB)
			{
				Is_Player_Ready[i] = 1;
				NetworkSendCommandDrawText(&Game_Players[i], "You are ready. Waiting for others...");
				printf("Player #%d is ready.\n", i + 1);
			}
		}
		
		// Are all connected players ready to start the game ?
		if (Game_Players_Count >= 2) // Almost 2 players are needed to start the game
		{
			// Are all players ready ?
			for (i = 0; i < Game_Players_Count; i++)
			{
				if (!Is_Player_Ready[i]) break;
			}
			if (i == Game_Players_Count) return; // All players are ready
		}
		
		// Wait some time to avoid 100% CPU usage
		nanosleep(&Waiting_Time, NULL);
	}
}

/** Send the map to all connected clients. */
static inline void GameDisplayMap(void)
{
	int i, Row, Column;
	
	for (i = 0; i < Game_Players_Count; i++)
	{
		for (Row = 0; Row < CONFIGURATION_MAP_ROWS_COUNT; Row++)
		{
			for (Column = 0; Column < CONFIGURATION_MAP_COLUMNS_COUNT; Column++) NetworkSendCommandDrawTile(&Game_Players[i], Map[Row][Column].Tile_ID, Row, Column);
		}
	}
}

/** Tell all clients to display the specified player (automatically choose the right player tile according to the client).
 * @param Pointer_Player The player to display.
 */
static inline void GameDisplayPlayer(TGamePlayer *Pointer_Player)
{
	int i;
	TGameTileID Tile_ID;
	
	for (i = 0; i < Game_Players_Count; i++)
	{
		// Select the right tile to send according to the destination client
		if (Game_Players[i].Socket == Pointer_Player->Socket) Tile_ID = GAME_TILE_ID_CURRENT_PLAYER;
		else Tile_ID = GAME_TILE_ID_OTHER_PLAYER;
	
		NetworkSendCommandDrawTile(&Game_Players[i], Tile_ID, Pointer_Player->Row, Pointer_Player->Column);
		
		// Display the shield on top of the player
		if (Pointer_Player->Shield_Timer > 0) NetworkSendCommandDrawTile(&Game_Players[i], GAME_TILE_SHIELD_OVERLAY, Pointer_Player->Row, Pointer_Player->Column);
	}
}

/** Send the tile to all players.
 * @param Tile_ID The tile to send.
 * @param Row The map Y cell coordinate where to display the tile.
 * @param Column The map X cell coordinate where to display the tile.
 */
static inline void GameDisplayTile(TGameTileID Tile_ID, int Row, int Column)
{
	int i;
	
	for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawTile(&Game_Players[i], Tile_ID, Row, Column);
}

/** Put all players on a different spawn point. */
static inline void GameSpawnPlayers(void)
{
	int i, Row, Column;
	
	Game_Alive_Players_Count = 0;
	
	for (i = 0; i < Game_Players_Count; i++)
	{
		// Do not spawn a disconnected player
		if (Game_Players[i].Socket == -1) continue;
		
		// Get next spawn point coordinates
		MapGetSpawnPointCoordinates(i, &Row, &Column);
	
		// Put player at this location
		Game_Players[i].Row = Row;
		Game_Players[i].Column = Column;
		Game_Players[i].Is_Alive = 1;
		Game_Alive_Players_Count++;
		
		// Initialize bombs
		Game_Players[i].Bombs_Count = 1;
		Game_Players[i].Explosion_Range = 2; // Take into account the explosion center too
		
		// Initialize shield
		Game_Players[i].Shield_Timer = 0;
		
		// Tell the clients to display the player
		GameDisplayPlayer(&Game_Players[i]);
	}
}

/** Tell if a player can move to a specific map cell.
 * @param Destination_Cell_Content The type of the destination cell.
 * @return 0 if the player can't go to this cell,
 * @return 1 if the player can reach this cell.
 */
static inline int GameIsPlayerMoveAllowed(TMapCellContent Destination_Cell_Content)
{
	// The player can't cross the walls
	if (Destination_Cell_Content == MAP_CELL_CONTENT_WALL) return 0;
	
	// The player can't cross a bomb
	if (Destination_Cell_Content == MAP_CELL_CONTENT_BOMB) return 0;
	
	// The player can't cross a destructible obstacle
	if (Destination_Cell_Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) return 0;

	return 1;
}

/** A player has just died. Notify him and take his death into account in the game mechanisms.
 * @param Pointer_Player The player that just died.
 */
static inline void GameSetPlayerDead(TGamePlayer *Pointer_Player)
{
	// Do not kill the player more than once
	if (!Pointer_Player->Is_Alive) return;
	
	NetworkSendCommandDrawText(Pointer_Player, "You are dead !");
	Pointer_Player->Is_Alive = 0;
	Game_Alive_Players_Count--;
	
	// TODO handle scoring
	
	printf("%s is dead.\n", Pointer_Player->String_Name);
}

/** Process a received event for a specific player.
 * @param Pointer_Player The player that has received an event.
 * @param Event The event received from the player.
 */
static inline void GameProcessEvents(TGamePlayer *Pointer_Player, TNetworkEvent Event)
{
	TMapCellContent Cell_Content;
	TMapCell *Pointer_Cell;
	int Has_Player_Moved = 0, Player_Previous_Row = 0, Player_Previous_Column = 0, i, Is_Player_Destination_Cell_Empty = 0;
	
	switch (Event)
	{
		case NETWORK_EVENT_GO_UP:
			// The player can't cross the map borders
			if (Pointer_Player->Row == 0) return;
			
			// Check if the move is allowed
			Cell_Content = Map[Pointer_Player->Row - 1][Pointer_Player->Column].Content;
			if (!GameIsPlayerMoveAllowed(Cell_Content)) return;
		
			Player_Previous_Row = Pointer_Player->Row;
			Player_Previous_Column = Pointer_Player->Column;
			Pointer_Player->Row--;
			Has_Player_Moved = 1;
			break;
			
		case NETWORK_EVENT_GO_DOWN:
			// The player can't cross the map borders
			if (Pointer_Player->Row == CONFIGURATION_MAP_ROWS_COUNT - 1) return;
			
			// Check if the move is allowed
			Cell_Content = Map[Pointer_Player->Row + 1][Pointer_Player->Column].Content;
			if (!GameIsPlayerMoveAllowed(Cell_Content)) return;
		
			Player_Previous_Row = Pointer_Player->Row;
			Player_Previous_Column = Pointer_Player->Column;
			Pointer_Player->Row++;
			Has_Player_Moved = 1;
			break;
			
		case NETWORK_EVENT_GO_LEFT:
			// The player can't cross the map borders
			if (Pointer_Player->Column == 0) return;
			
			// Check if the move is allowed
			Cell_Content = Map[Pointer_Player->Row][Pointer_Player->Column - 1].Content;
			if (!GameIsPlayerMoveAllowed(Cell_Content)) return;
			
			Player_Previous_Row = Pointer_Player->Row;
			Player_Previous_Column = Pointer_Player->Column;
			Pointer_Player->Column--;
			Has_Player_Moved = 1;
			break;
			
		case NETWORK_EVENT_GO_RIGHT:
			// The player can't cross the map borders
			if (Pointer_Player->Column == CONFIGURATION_MAP_COLUMNS_COUNT - 1) return;
			
			// Check if the move is allowed
			Cell_Content = Map[Pointer_Player->Row][Pointer_Player->Column + 1].Content;
			if (!GameIsPlayerMoveAllowed(Cell_Content)) return;
			
			Player_Previous_Row = Pointer_Player->Row;
			Player_Previous_Column = Pointer_Player->Column;
			Pointer_Player->Column++;
			Has_Player_Moved = 1;
			break;
			
		case NETWORK_EVENT_DROP_BOMB:
			// Can the player drop a bomb ?
			if (Pointer_Player->Bombs_Count == 0) return;
			
			// Try to drop the bomb at player location
			if (GameDropBomb(Pointer_Player->Row, Pointer_Player->Column, Pointer_Player->Explosion_Range, Pointer_Player) != 0) return;
			
			// Display the bomb
			GameDisplayTile(GAME_TILE_BOMB, Pointer_Player->Row, Pointer_Player->Column);
			// Redraw the player on top of the bomb
			GameDisplayPlayer(Pointer_Player);
			
			Pointer_Player->Bombs_Count--;
			break;
		
		case NETWORK_EVENT_DISCONNECT:
			GameRemoveDisconnectedPlayer(Pointer_Player);
			break;
			
		default:
			printf("[%s:%d] Warning : unknown event (%d) from socket %d.\n", __FUNCTION__, __LINE__, Event, Pointer_Player->Socket);
			break;
	}
	
	// Notify all clients that a player moved
	if (Has_Player_Moved)
	{
		// Cache the cell address
		Pointer_Cell = &Map[Pointer_Player->Row][Pointer_Player->Column];
		
		// Is the cell exploding ?
		if ((Pointer_Cell->Explosion_State == MAP_EXPLOSION_STATE_REMOVE_EXPLOSION_TILE) && (Pointer_Player->Shield_Timer == 0))
		{
			GameSetPlayerDead(Pointer_Player);
			// Remove the player trace from all clients
			GameDisplayTile(GAME_TILE_ID_EMPTY, Player_Previous_Row, Player_Previous_Column);
			return;
		}
		
		// Get item if there is one on the cell
		switch (Cell_Content)
		{
			case MAP_CELL_CONTENT_ITEM_SHIELD:
				Pointer_Player->Shield_Timer = CONFIGURATION_SHIELD_DURATION_TIME;
				Pointer_Cell->Content = MAP_CELL_CONTENT_EMPTY;
				break;
				
			case MAP_CELL_CONTENT_ITEM_POWER_UP_BOMB_RANGE:
				Pointer_Player->Explosion_Range++;
				Pointer_Cell->Content = MAP_CELL_CONTENT_EMPTY;
				break;
				
			case MAP_CELL_CONTENT_ITEM_POWER_UP_BOMBS_COUNT:
				Pointer_Player->Bombs_Count++;
				Pointer_Cell->Content = MAP_CELL_CONTENT_EMPTY;
				break;
				
			// This is not a retrievable item
			default:
				Is_Player_Destination_Cell_Empty = 1;
				break;
		}
		
		// Tell all clients to erase the player trace (previous trace must always be erased because some player tile is thinner than other and superposition is visible)
		GameDisplayTile(GAME_TILE_ID_EMPTY, Player_Previous_Row, Player_Previous_Column);
		
		// Display a bomb if there was one here
		if (Map[Player_Previous_Row][Player_Previous_Column].Content == MAP_CELL_CONTENT_BOMB) GameDisplayTile(GAME_TILE_BOMB, Player_Previous_Row, Player_Previous_Column);
		
		// Clear the cell the player is on if it contained an item in order to make this item disappear
		if (!Is_Player_Destination_Cell_Empty) GameDisplayTile(GAME_TILE_ID_EMPTY, Pointer_Player->Row, Pointer_Player->Column);
		
		// Display other players if they were here too
		for (i = 0; i < Game_Players_Count; i++)
		{
			if ((Game_Players[i].Is_Alive) && (Game_Players[i].Socket != Pointer_Player->Socket) && (Game_Players[i].Row == Player_Previous_Row) && (Game_Players[i].Column == Player_Previous_Column))
			{
				GameDisplayPlayer(&Game_Players[i]); // As all enemy players are identical, only one must be drawn even if several players are located on the same map cell
				break;
			}
		}

		// Draw the player at his new location
		GameDisplayPlayer(Pointer_Player);
	}
}

/** Browse the map to find which cells must explode.
 * @note The function must be called exactly at each game tick.
 */
static inline void GameHandleBombs(void)
{
	int Row, Column, i;
	TMapCell *Pointer_Cell;
	TGameTileID Tile_ID;
	
	for (Row = 0; Row < CONFIGURATION_MAP_ROWS_COUNT; Row++)
	{
		for (Column = 0; Column < CONFIGURATION_MAP_COLUMNS_COUNT; Column++)
		{
			// Cache cell address
			Pointer_Cell = &Map[Row][Column];
			
			// Bypass cells that are not involved in an explosion
			if (Pointer_Cell->Explosion_State == MAP_EXPLOSION_STATE_NO_BOMB) continue;
			
			// Is it time to take an action ?
			if (Pointer_Cell->Explosion_Timer > 0)
			{
				Pointer_Cell->Explosion_Timer--;
				continue;
			}
			
			// Display the explosion
			if (Pointer_Cell->Explosion_State == MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE)
			{
				Tile_ID = GAME_TILE_EXPLOSION;
				Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_REMOVE_EXPLOSION_TILE;
				
				// Reschedule the timer
				Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME;
				
				// Handle player collision with bomb flames
				// Is there one or more player(s) on this cell ?
				for (i = 0; i < Game_Players_Count; i++)
				{
					if ((Game_Players[i].Row == Row) && (Game_Players[i].Column == Column) && (Game_Players[i].Shield_Timer == 0)) GameSetPlayerDead(&Game_Players[i]);
				}
			}
			// The explosion has just finished
			else
			{
				Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_NO_BOMB;
				
				// Was this cell containing the bomb ?
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_BOMB)
				{
					// Remove the bomb from the map
					Pointer_Cell->Content = MAP_CELL_CONTENT_EMPTY;
					
					// If it was a player that dropped the bomb, he has now a new ready bomb
					if (Pointer_Cell->Pointer_Owner_Player != NULL) Pointer_Cell->Pointer_Owner_Player->Bombs_Count++; // This pointer is valid only for the cell containing the bomb itself and if the bomb was dropped by a player (not spawned as a random item)
					
					Tile_ID = GAME_TILE_ID_EMPTY;
				}
				// Remove a destructible obstacle
				else if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE)
				{
					// Randomly spawn an item (or nothing)
					MapSpawnItem(Row, Column);
					
					Tile_ID = Pointer_Cell->Tile_ID;
				}
				// The cell was empty, let it empty
				else Tile_ID = GAME_TILE_ID_EMPTY;
			}
			
			// Tell all clients to display the sprite
			GameDisplayTile(Tile_ID, Row, Column);
			
			// Check if a player protected by a shield was on this cell when the explosion is terminated
			if (Pointer_Cell->Explosion_State == MAP_EXPLOSION_STATE_NO_BOMB) // The bomb just finished to explode
			{
				for (i = 0; i < Game_Players_Count; i++)
				{
					if ((Game_Players[i].Is_Alive) && (Game_Players[i].Row == Row) && (Game_Players[i].Column == Column) && (Game_Players[i].Shield_Timer > 0)) GameDisplayPlayer(&Game_Players[i]); // Display all players in connection order
				}
			}
		}
	}
}

/** Handle all player shields.
 * @note The function must be called exactly at each game tick.
 */
static inline void GameHandleShields(void)
{
	int i;
	TGamePlayer *Pointer_Player;
	
	for (i = 0; i < Game_Players_Count; i++)
	{
		// Cache the player address
		Pointer_Player = &Game_Players[i];
		
		if (Pointer_Player->Shield_Timer > 0)
		{
			Pointer_Player->Shield_Timer--;
			
			// Remove the player shield if it timed out (the tile must be refreshed if the player does not move)
			if (Pointer_Player->Shield_Timer == 0)
			{
				GameDisplayTile(GAME_TILE_ID_EMPTY, Pointer_Player->Row, Pointer_Player->Column);
				GameDisplayPlayer(Pointer_Player);
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
int GameLoop(void)
{
	int i, Map_Spawn_Points_Count;
	TNetworkEvent Event;
	struct timespec Time_To_Wait;
	char String_Next_Round_Message[CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH + 64]; // 64 bytes are enough for the static text
	
	// Make sure everyone is in before starting the game
	GameWaitForPlayersConnection();
	Game_Connected_Players_Count = Game_Players_Count;
	
	// Start a game
	while (1)
	{
		// Are there at least 2 players to make the game works ?
		if (Game_Connected_Players_Count < 2)
		{
			// Inform all remaining players to quit
			for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawText(&Game_Players[i], "Not enough players remaining, please quit the server to make it restart a game.");
			printf("Only %d player remaining, restarted server.\n", Game_Connected_Players_Count);
			return 0;
		}
		
		// Try to load a map
		if (MapLoadRandom() != 0)
		{
			printf("[%s:%d] Error : failed to load the map.\n", __FUNCTION__, __LINE__);
			return 1;
		}
	
		// Are there enough spawn points for all players ?
		Map_Spawn_Points_Count = MapGetSpawnPointsCount();
		if (Map_Spawn_Points_Count < Game_Players_Count)
		{
			printf("[%s:%d] Error : the map has only %d spawn points while %d players are expected.\n", __FUNCTION__, __LINE__, Map_Spawn_Points_Count, Game_Players_Count);
			return 1;
		}
		printf("Map successfully loaded.\n");
		
		// Send the map to all players
		GameDisplayMap();
		printf("Map sent to players.\n");
		
		// Choose initial players location
		GameSpawnPlayers();
		printf("Players spawned.\n");
		
		// Tell all clients that game is ready
		for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawText(&Game_Players[i], "Go !");
		printf("Launching game.\n");
		
		// Update player actions and bombs
		while (1)
		{
			// Get loop starting time
			if (clock_gettime(CLOCK_MONOTONIC, &Time_To_Wait) != 0) printf("[%s:%d] Error : clock_gettime() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
			
			// Add the required waiting time
			Time_To_Wait.tv_nsec = (Time_To_Wait.tv_nsec + CONFIGURATION_GAME_TICK) % 999999999; // The maximum nanoseconds value is 999999999
			if (Time_To_Wait.tv_nsec < CONFIGURATION_GAME_TICK) Time_To_Wait.tv_sec++; // Adjust seconds if nanoseconds overlapped
			
			// Handle player events
			for (i = 0; i < Game_Players_Count; i++)
			{
				// Ignore dead players
				if (!Game_Players[i].Is_Alive) continue;
				
				if (NetworkGetEvent(&Game_Players[i], &Event) != 0) printf("[%s:%d] Error : failed to get the player #%d next event.\n", __FUNCTION__, __LINE__, i + 1);
				else if (Event != NETWORK_EVENT_NONE) GameProcessEvents(&Game_Players[i], Event); // Avoid calling the function if there is nothing to do
			}
			
			// Handle bombs now that players may have moved to grant them more chances of survival
			GameHandleBombs();
			
			GameHandleShields();
			
			// Exit game if there is only one (or zero) player remaining
			if (Game_Connected_Players_Count < 2) break;
			
			// Is there a last player standing ?
			if (Game_Alive_Players_Count <= 1) // One player remaining or all players dead
			{
				if (Game_Alive_Players_Count == 1)
				{
					// Find this player
					for (i = 0; i < Game_Players_Count; i++)
					{
						if (Game_Players[i].Is_Alive) break;
					}
					
					// Tell all players that he won
					sprintf(String_Next_Round_Message, "%s has won ! %d seconds before next round...", Game_Players[i].String_Name, CONFIGURATION_SECONDS_BETWEEN_NEXT_ROUND);
				}
				else sprintf(String_Next_Round_Message, "Everyone died. %d seconds before next round...", CONFIGURATION_SECONDS_BETWEEN_NEXT_ROUND);
				
				// Send the message to all players
				for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawText(&Game_Players[i], String_Next_Round_Message);
				printf("%s\n", String_Next_Round_Message);
				
				usleep(CONFIGURATION_SECONDS_BETWEEN_NEXT_ROUND * 1000000);
				break;
			}
			else
			{
				// Wait for the required absolute time
				if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &Time_To_Wait, NULL) != 0) printf("[%s:%d] Error : clock_nanosleep() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
			}
		}
	}
}

int GameDropBomb(int Row, int Column, int Explosion_Range, TGamePlayer *Pointer_Owner_Player)
{
	TMapCell *Pointer_Cell;
	int Explosion_Row, Explosion_Column, i;
	
	// Cache the cell address
	Pointer_Cell = &Map[Row][Column];
	
	// Only one bomb can be placed in a cell
	if (Pointer_Cell->Content == MAP_CELL_CONTENT_BOMB) return 1;
	
	// Put the bomb at this location on the map
	Pointer_Cell->Content = MAP_CELL_CONTENT_BOMB;
	// Store whom player dropped the bomb
	Pointer_Cell->Pointer_Owner_Player = Pointer_Owner_Player;
	
	// Initialize the explosion timers
	// Explosion center (the cell where the bomb is dropped)
	Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER;
	Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
	
	// Center to up explosion propagation
	Explosion_Row = Row;
	for (i = 1; i < Explosion_Range; i++) // Start from 1 to bypass the explosion center (no need to set it more than once)
	{
		// Stop when hitting the map border
		Explosion_Row--;
		if (Explosion_Row < 0) break;
		
		// Stop when hitting a wall
		Pointer_Cell = &Map[Explosion_Row][Column]; // Cache cell address for a faster access
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
		
		// Program the explosion
		Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
		Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
		
		// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
	}
	
	// Center to down explosion propagation
	Explosion_Row = Row;
	for (i = 1; i < Explosion_Range; i++)
	{
		// Stop when hitting the map border
		Explosion_Row++;
		if (Explosion_Row >= CONFIGURATION_MAP_ROWS_COUNT) break;
		
		// Stop when hitting a wall
		Pointer_Cell = &Map[Explosion_Row][Column]; // Cache cell address for a faster access
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
		
		// Program the explosion
		Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
		Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
		
		// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
	}
	
	// Center to left explosion propagation
	Explosion_Column = Column;
	for (i = 1; i < Explosion_Range; i++)
	{
		// Stop when hitting the map border
		Explosion_Column--;
		if (Explosion_Column < 0) break;
		
		// Stop when hitting a wall
		Pointer_Cell = &Map[Row][Explosion_Column]; // Cache cell address for a faster access
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
		
		// Program the explosion
		Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
		Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
		
		// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
	}
	
	// Center to right explosion propagation
	Explosion_Column = Column;
	for (i = 1; i < Explosion_Range; i++)
	{
		// Stop when hitting the map border
		Explosion_Column++;
		if (Explosion_Column >= CONFIGURATION_MAP_COLUMNS_COUNT) break;
		
		// Stop when hitting a wall
		Pointer_Cell = &Map[Row][Explosion_Column]; // Cache cell address for a faster access
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
		
		// Program the explosion
		Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
		Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
		
		// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
		if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
	}
	
	return 0;
}

void GameRemoveDisconnectedPlayer(TGamePlayer *Pointer_Player)
{
	// Close the connection first to avoid sending data to the non-existing client
	close(Pointer_Player->Socket);
	Pointer_Player->Socket = -1; // Tell the Network functions to ignore this client
	
	// Consider the player as dead
	GameSetPlayerDead(Pointer_Player);
	
	// Remove the player tile from the map
	GameDisplayTile(GAME_TILE_ID_EMPTY, Pointer_Player->Row, Pointer_Player->Column);
			
	Game_Connected_Players_Count--;
	printf("%s leaved.\n", Pointer_Player->String_Name);
}
