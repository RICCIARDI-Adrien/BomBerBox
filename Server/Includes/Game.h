/** @file Game.h
 * Process game events an update clients.
 * @author Adrien RICCIARDI
 */

#ifndef H_GAME_H
#define H_GAME_H

//-------------------------------------------------------------------------------------------------
// Types
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
	int Is_Alive; //!< Tell if the player is alive or not.
	// TODO bonus items
	int Is_Ghost_Mode_Enabled; //!< Tell if the player can cross the destructible objects or not.
} TGamePlayer;

/** All available tiles. */
typedef enum
{
	GAME_TILE_ID_EMPTY,
	GAME_TILE_ID_WALL,
	GAME_TILE_ID_DESTRUCTIBLE_OBSTACLE,
	GAME_TILE_ID_CURRENT_PLAYER,
	GAME_TILE_ID_OTHER_PLAYER,
	GAME_TILE_BOMB,
	GAME_TILE_EXPLOSION,
	GAME_TILE_ITEM_GHOST,
	GAME_TILE_ITEM_SHIELD,
	GAME_TILE_ITEM_POWER_UP_BOMB_RANGE,
	GAME_TILE_ITEM_POWER_UP_BOMBS_COUNT
} TGameTileID;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Wait for the players to connect and start the game.
 * @param Expected_Players_Count How many players to wait for.
 */
void GameLoop(int Expected_Players_Count);

#endif
