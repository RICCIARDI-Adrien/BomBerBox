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
	MAP_CELL_CONTENT_BOMB, //!< A bomb is here.
	MAP_CELL_CONTENT_ITEM_SHIELD, //!< The cell contains a shield.
	MAP_CELL_CONTENT_ITEM_POWER_UP_BOMB_RANGE, //!< Improve the player bombs explosion range.
	MAP_CELL_CONTENT_ITEM_POWER_UP_BOMBS_COUNT //!< The player can carry one more bomb.
} TMapCellContent;

/** All states an exploding cell can take. */
typedef enum
{
	MAP_EXPLOSION_STATE_NO_BOMB, //!< There is no bomb in this cell.
	MAP_EXPLOSION_STATE_DISPLAY_EXPLOSION_TILE, //!< The bomb has just exploded, display the explosion.
	MAP_EXPLOSION_STATE_REMOVE_EXPLOSION_TILE //!< The explosion finished, remove the explosion tile.
} TMapExplosionState;

/** A map cell. */
typedef struct
{
	TMapCellContent Content; //!< What is located in the cell.
	TGameTileID Tile_ID; //!< The corresponding tile.
	TMapExplosionState Explosion_State; //!< What the handling bomb routine must do with this cell.
	int Explosion_Timer; //!< The handling bomb routine will take the action described by Explosion_State when this counter reached 0.
	TGamePlayer *Pointer_Owner_Player; //!< The player who dropped the bomb.
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

/** Randomly spawn an item (or nothing) at the specified location.
 * @param Row The Y location.
 * @param Column The X location.
 */
void MapSpawnItem(int Row, int Column);

#endif