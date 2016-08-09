/** @file Game.h
 * Process game events an update clients.
 * @author Adrien RICCIARDI
 */

#ifndef H_GAME_H
#define H_GAME_H

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
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
	GAME_TILE_ITEM_POWER_UP,
	GAME_TILE_ITEM_SHIELD
} TGameTileID;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Wait for the players to connect and start the game.
 * @param Expected_Players_Count How many players to wait for.
 */
void GameLoop(int Expected_Players_Count);

#endif