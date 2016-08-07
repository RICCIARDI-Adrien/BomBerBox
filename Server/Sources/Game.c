/** @file Game.c
 * @see Game.h for description.
 * @author Adrien RICCIARDI
 */

#include <Configuration.h>
#include <Game.h>
#include <Map.h>
#include <Network.h>
#include <stdio.h>

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
/** A player attributes. */
typedef struct
{
	char Name[CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH]; //!< The player name.
	int Socket; //<! The network socket used to communicate with the client.
	int Row; //!< The player Y location on the map.
	int Column; //!< The player X location on the map.
	int Is_Bomb_Available; //!< Tell if the player can use a bomb or not.
	int Explosion_Range; //!< How many cells an explosion can reach.
	// TODO bonus items
} TGamePlayer;

/** A bomb item. */
typedef struct
{
	//TODO Explosion_Timer
	int Row; //!< The bomb Y location on the map.
	int Column; //!< The bomb X location on the map.
	int Owning_Player_ID; //!< Tell which player thrown the bomb.
} TGameBomb;

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

/** Process a received event for a specific player.
 * @param Pointer_Player The player that has received an event.
 * @param Event The event received from the player.
 */
static inline void GameProcessEvents(TGamePlayer *Pointer_Player, TNetworkEvent Event)
{
	switch (Event)
	{
		case NETWORK_EVENT_GO_UP:
			printf("up\n"); // TEST
			break;
			
		default:
			printf("unknown event\n"); // TEST
	}
}

/*GameSpawnPlayers

GameMovePlayer*/

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
	
	// TODO spawn players
	// TODO tell all clients game is ready (remove previous text too)
	
	while (1) // TEST, TODO clean exit
	{
		// TODO tick timer
		
		// Handle player events
		for (i = 0; i < Game_Players_Count; i++)
		{
			if (NetworkGetEvent(Game_Players[i].Socket, &Event) != 0) printf("[%s:%d] Error : failed to get the player #%d next event.\n", __FUNCTION__, __LINE__, i + 1);
			else GameProcessEvents(&Game_Players[i], Event);
		}
		
		// TODO handle bombs
		
		// TODO tick timer
	}
}