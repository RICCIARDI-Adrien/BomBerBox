/** @file Game.c
 * @see Game.h for description.
 * @author Adrien RICCIARDI
 */

#include <Configuration.h>
#include <Game.h>
#include <Map.h>
#include <Network.h>
#include <stdio.h>
#include <time.h>

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** All players. */
static TGamePlayer Game_Players[CONFIGURATION_MAXIMUM_PLAYERS_COUNT];
/** How many players in the game. */
static int Game_Players_Count;

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
/** Wait for all clients to connect. */
static inline void GameWaitForPlayersConnection(void)
{
	int i;
	
	for (i = 0; i < Game_Players_Count; i++)
	{
		// Wait for a client connection
		if (NetworkWaitForPlayerConnection(&Game_Players[i].Socket, Game_Players[i].Name) != 0)
		{
			printf("[%s:%d] Error : client #%d failed to connect.\n", __FUNCTION__, __LINE__, i + 1);
			i--; // Do as if nothing happened to avoid kicking the other clients by stopping the server
			continue;
		}
		
		// Tell the client to wait for others
		NetworkSendCommandDrawText(Game_Players[i].Socket, "Successfully connected. Waiting for others...");
		
		printf("Client #%d connected, name : %s.\n", i + 1, Game_Players[i].Name);
	}
}

/** Send the map to all connected clients. */
static inline void GameInitializeMap(void)
{
	int i, Row, Column;
	
	for (i = 0; i < Game_Players_Count; i++)
	{
		for (Row = 0; Row < CONFIGURATION_MAP_ROWS_COUNT; Row++)
		{
			for (Column = 0; Column < CONFIGURATION_MAP_COLUMNS_COUNT; Column++) NetworkSendCommandDrawTile(Game_Players[i].Socket, Map[Row][Column].Tile_ID, Row, Column);
		}
	}
}

/** Put all players on a different spawn point. */
static inline void GameSpawnPlayers(void)
{
	int i, Row, Column, j;
	TGameTileID Tile_ID;
	
	for (i = 0; i < Game_Players_Count; i++)
	{
		// Get next spawn point coordinates
		MapGetSpawnPointCoordinates(i, &Row, &Column);
	
		// Put player at this location
		Game_Players[i].Row = Row;
		Game_Players[i].Column = Column;
		
		// Initialize bombs
		Game_Players[i].Is_Bomb_Available = 1;
		Game_Players[i].Explosion_Range = 10; // TEST, TODO set to 1
		
		// Tell the clients to display the player
		for (j = 0; j < Game_Players_Count; j++)
		{
			// Choose the right player tile according to the client
			if (j == i) Tile_ID = GAME_TILE_ID_CURRENT_PLAYER; // The client must recognize it's own player
			else Tile_ID = GAME_TILE_ID_OTHER_PLAYER;
			
			NetworkSendCommandDrawTile(Game_Players[j].Socket, Tile_ID, Row, Column);
		}
	}
}

/** Tell if a player can move to a specific map cell.
 * @param Pointer_Player The player.
 * @param Destination_Cell_Content The type of the destination cell.
 * @return 0 if the player can't go to this cell,
 * @return 1 if the player can reach this cell.
 */
static inline int GameIsPlayerMoveAllowed(TGamePlayer *Pointer_Player, TMapCellContent Destination_Cell_Content)
{
	// The player can't cross the walls
	if (Destination_Cell_Content == MAP_CELL_CONTENT_WALL) return 0;
	
	// The player can't cross a bomb
	if (Destination_Cell_Content == MAP_CELL_CONTENT_BOMB) return 0;
	
	// Is the player in ghost mode ?
	if ((Destination_Cell_Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) && (!Pointer_Player->Is_Ghost_Mode_Enabled)) return 0;

	return 1;
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
	
		NetworkSendCommandDrawTile(Game_Players[i].Socket, Tile_ID, Pointer_Player->Row, Pointer_Player->Column);
	}
}

/** Process a received event for a specific player.
 * @param Pointer_Player The player that has received an event.
 * @param Event The event received from the player.
 */
static inline void GameProcessEvents(TGamePlayer *Pointer_Player, TNetworkEvent Event)
{
	TMapCellContent Cell_Content;
	TMapCell *Pointer_Cell;
	int Has_Player_Moved = 0, Player_Previous_Row = 0, Player_Previous_Column = 0, i, Row, Column;
	
	switch (Event)
	{
		case NETWORK_EVENT_GO_UP:
			// The player can't cross the map borders
			if (Pointer_Player->Row == 0) return;
			
			// Check if the move is allowed
			Cell_Content = Map[Pointer_Player->Row - 1][Pointer_Player->Column].Content;
			if (!GameIsPlayerMoveAllowed(Pointer_Player, Cell_Content)) return;
		
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
			if (!GameIsPlayerMoveAllowed(Pointer_Player, Cell_Content)) return;
		
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
			if (!GameIsPlayerMoveAllowed(Pointer_Player, Cell_Content)) return;
			
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
			if (!GameIsPlayerMoveAllowed(Pointer_Player, Cell_Content)) return;
			
			Player_Previous_Row = Pointer_Player->Row;
			Player_Previous_Column = Pointer_Player->Column;
			Pointer_Player->Column++;
			Has_Player_Moved = 1;
			break;
			
		case NETWORK_EVENT_DROP_BOMB:
			// Can the player drop a bomb ?
			if (!Pointer_Player->Is_Bomb_Available) return;
			
			// Cache the cell address
			Pointer_Cell = &Map[Pointer_Player->Row][Pointer_Player->Column];
			
			// Only one bomb can be placed in a cell
			if (Pointer_Cell->Content == MAP_CELL_CONTENT_BOMB) return;
			
			// Put the bomb at this location on the map
			Pointer_Cell->Content = MAP_CELL_CONTENT_BOMB;
			// Store whom player dropped the bomb
			Pointer_Cell->Pointer_Owner_Player = Pointer_Player;
			
			// Initialize the explosion timers
			// Explosion center (the cell where the bomb is dropped)
			Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER;
			Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
			
			// Center to up explosion propagation
			Row = Pointer_Player->Row;
			for (i = 1; i <= Pointer_Player->Explosion_Range; i++) // Start from 1 to bypass the explosion center (no need to set it more than once)
			{
				// Stop when hitting the map border
				Row--;
				if (Row < 0) break;
				
				// Stop when hitting a wall
				Pointer_Cell = &Map[Row][Pointer_Player->Column]; // Cache cell address for a faster access
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
				
				// Program the explosion
				Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
				Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
				
				// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
			}
			
			// Center to down explosion propagation
			Row = Pointer_Player->Row;
			for (i = 1; i <= Pointer_Player->Explosion_Range; i++) // Start from 1 to bypass the explosion center (no need to set it more than once)
			{
				// Stop when hitting the map border
				Row++;
				if (Row >= CONFIGURATION_MAP_ROWS_COUNT) break;
				
				// Stop when hitting a wall
				Pointer_Cell = &Map[Row][Pointer_Player->Column]; // Cache cell address for a faster access
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
				
				// Program the explosion
				Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
				Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
				
				// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
			}
			
			// Center to left explosion propagation
			Column = Pointer_Player->Column;
			for (i = 1; i <= Pointer_Player->Explosion_Range; i++)
			{
				// Stop when hitting the map border
				Column--;
				if (Column < 0) break;
				
				// Stop when hitting a wall
				Pointer_Cell = &Map[Pointer_Player->Row][Column]; // Cache cell address for a faster access
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
				
				// Program the explosion
				Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
				Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
				
				// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
			}
			
			// Center to right explosion propagation
			Column = Pointer_Player->Column;
			for (i = 1; i <= Pointer_Player->Explosion_Range; i++)
			{
				// Stop when hitting the map border
				Column++;
				if (Column >= CONFIGURATION_MAP_COLUMNS_COUNT) break;
				
				// Stop when hitting a wall
				Pointer_Cell = &Map[Pointer_Player->Row][Column]; // Cache cell address for a faster access
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_WALL) break;
				
				// Program the explosion
				Pointer_Cell->Explosion_Timer = CONFIGURATION_BOMB_EXPLOSION_TIMER + (i * CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME);
				Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE;
				
				// Stop after having hit a destructible obstacle (only one destructible obstacle must be destroyed by the bomb)
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE) break;
			}
			
			// Display the bomb
			for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawTile(Game_Players[i].Socket, GAME_TILE_BOMB, Pointer_Player->Row, Pointer_Player->Column);
			// Redraw the player on top of the bomb
			GameDisplayPlayer(Pointer_Player);
			
			Pointer_Player->Is_Bomb_Available = 0;
			break;
			
		// TODO handle player deconnection (set player socket to -1 to disable network command functions)
			
		default:
			printf("[%s:%d] Warning : unknown event (%d) from socket %d.\n", __FUNCTION__, __LINE__, Event, Pointer_Player->Socket);
			break;
	}
	
	// Notify all clients that a player moved
	if (Has_Player_Moved)
	{
		// TODO get item if there is one on the cell
		
		// Tell all clients to erase the player trace (prevous trace must always be erased because some player tile is thinner than other and superposition is visible)
		for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawTile(Game_Players[i].Socket, GAME_TILE_ID_EMPTY, Player_Previous_Row, Player_Previous_Column);
		
		// Display a bomb if there was one here
		if (Map[Player_Previous_Row][Player_Previous_Column].Content == MAP_CELL_CONTENT_BOMB)
		{
			for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawTile(Game_Players[i].Socket, GAME_TILE_BOMB, Player_Previous_Row, Player_Previous_Column);
		}
		
		// Display other players if they were here too
		for (i = 0; i < Game_Players_Count; i++)
		{
			if ((Game_Players[i].Socket != Pointer_Player->Socket) && (Game_Players[i].Row == Player_Previous_Row) && (Game_Players[i].Column == Player_Previous_Column))
			{
				GameDisplayPlayer(&Game_Players[i]); // As all enemy players are identical, only one must be drawn even if several players are located on the same map cell
				break;
			}
		}

		// Draw the player at his new location
		GameDisplayPlayer(Pointer_Player);
	}
}

// TODO
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
				Pointer_Cell->Explosion_Timer = 1000000;
			}
			else
			{
				Tile_ID = GAME_TILE_ID_EMPTY;
				Pointer_Cell->Explosion_State = MAP_EXPLOSION_STATE_NO_BOMB;
				
				// Was this cell containing the bomb ?
				if (Pointer_Cell->Content == MAP_CELL_CONTENT_BOMB)
				{
					// Remove the bomb from the map
					Pointer_Cell->Content = MAP_CELL_CONTENT_EMPTY;
					
					// The player has now a new ready bomb
					Pointer_Cell->Pointer_Owner_Player->Is_Bomb_Available = 1; // This pointer is valid only for the cell containing the bomb itself
				}
			}
			
			// Tell all clients to display the sprite
			for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawTile(Game_Players[i].Socket, Tile_ID, Row, Column);
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void GameLoop(int Expected_Players_Count)
{
	int i;
	TNetworkEvent Event;
	
	// Make sure everyone is in before starting the game
	Game_Players_Count = Expected_Players_Count;
	GameWaitForPlayersConnection();
	
	// All players are in, send them the map
	GameInitializeMap();
	printf("Map sent to players.\n");
	
	// Choose initial players location
	GameSpawnPlayers();
	printf("Players spawned.\n");
	
	// Tell all clients that game is ready
	for (i = 0; i < Game_Players_Count; i++) NetworkSendCommandDrawText(Game_Players[i].Socket, "Go !");
	printf("Launching game.\n");
	
	while (1) // TODO clean exit
	{
		// TODO tick timer (better in GameHandleBombs() ?)
				
		// Handle player events
		for (i = 0; i < Game_Players_Count; i++)
		{
			if (NetworkGetEvent(Game_Players[i].Socket, &Event) != 0) printf("[%s:%d] Error : failed to get the player #%d next event.\n", __FUNCTION__, __LINE__, i + 1);
			else if (Event != NETWORK_EVENT_NONE) GameProcessEvents(&Game_Players[i], Event); // Avoid calling the function if there is nothing to do
		}
		
		// Handle bombs now that players may have moved to grant them more chances of survival
		GameHandleBombs();
		
		// TODO Check player collision with bomb
		
		// TODO tick timer (better in GameHandleBombs() ?)
	}
}
