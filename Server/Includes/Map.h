/** @file Map.h
 * Load and access to a map content.
 * @author Adrien RICCIARDI
 */

#ifndef H_MAP_H
#define H_MAP_H

#include <Configuration.h>
#include <Game.h>

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** What a cell can content (players and bombs are handled apart). */
typedef enum
{
	MAP_CELL_CONTENT_EMPTY, //!< No item or wall in the cell.
	MAP_CELL_CONTENT_WALL, //!< There is an indestructible wall in the cell.
	MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE, //!< The obstacle can be broken by a bomb.
	MAP_CELL_CONTENT_PLAYER_SPAWN_POINT, //!< A player can spawn here.
	MAP_CELL_CONTENT_NO_DESTRUCTIBLE_OBSTACLE_ZONE, //!< No destructible obstacle can be spawn here.
	MAP_CELL_CONTENT_BOMB //!< A bomb is here.
} TMapCellContent;

// TODO
typedef enum
{
	MAP_EXPLOSION_STATE_NO_BOMB,
	MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE,
	MAP_EXPLOSION_STATE_REMOVE_EXPLOSION_TILE
} TMapExplosionState;

/** A map cell. */
typedef struct
{
	TMapCellContent Content; //!< What is located in the cell.
	TGameTileID Tile_ID; //!< The corresponding tile.
	//int Is_Exploding; //!< Tell if this cell is currently exploding or not.
	TMapExplosionState Explosion_State;
	int Explosion_Timer; // TODO
} TMapCell;

//-------------------------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------------------------
/** The map itself. */
extern TMapCell Map[CONFIGURATION_MAP_ROWS_COUNT][CONFIGURATION_MAP_COLUMNS_COUNT];

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Load a map from a text file.
 * @param String_File_Path The file location.
 * @return 0 if the map was successfully loaded,
 * @return 1 if an error occurred.
 */
int MapLoad(char *String_File_Path);

/** Tell how many spawn points the map has.
 * @return The spawn points amount.
 */
int MapGetSpawnPointsCount(void);

/** Get a specified spawn point coordinates. Spawn points are numbered starting from map left to right, upper to bottom.
 * @param Spawn_Point_Index The spawn point index (leftmost and upper map spawn point is 0, index increments continuing to right then to next row).
 * @param Pointer_Row On output, contain the spawn point row.
 * @param Pointer_Column On output, contain the spawn point column.
 * @note If the spawn point index does not exist, the returned coordinates will be zero.
 */
void MapGetSpawnPointCoordinates(int Spawn_Point_Index, int *Pointer_Row, int *Pointer_Column);

// MapSpawnItem 

#endif