/** @file Game.h
 * Process game events an update clients.
 * @author Adrien RICCIARDI
 */

#ifndef H_GAME_H
#define H_GAME_H

#include <Configuration.h>

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** A player attributes. */
typedef struct
{
	char String_Name[CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH]; //!< The player name.
	int Socket; //!< The network socket used to communicate with the client.
	int Row; //!< The player Y location on the map.
	int Column; //!< The player X location on the map.
	int Bombs_Count; //!< Tell how many bombs the player can carry.
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
 * @return 0 if no error occurred,
 * @return 1 if an error occurred.
 */
int GameLoop(void);

/** Drop a bomb at the specified location on the map.
 * @param Row The Y map cell location.
 * @param Column The X map cell location.
 * @param Explosion_Range How far the bomb will explode (1 = only the cell where the bomb is dropped, 2 = the cell containing the bomb plus one cell on each corner, ...).
 * @param Pointer_Owner_Player Set to the player pointer if it was a player that dropped the bomb, set to NULL if the bomb was spawned as an item.
 * @return 0 if the bomb was successfully dropped,
 * @return 1 if the bomb could not be dropped.
 */
int GameDropBomb(int Row, int Column, int Explosion_Range, TGamePlayer *Pointer_Owner_Player);

#endif
