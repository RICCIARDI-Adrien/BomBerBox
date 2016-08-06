/** @file Map.h
 * Load and access to a map content.
 * Map file format :
 * The map file is an ASCII text file that must exactly fit CONFIGURATION_MAP_ROWS_COUNT lines of CONFIGURATION_MAP_COLUMNS_COUNT columns.
 * The following characters have meaning :
 *   ' ' (space) : an empty cell
 *   D : a destructible object
 *   S : a player spawn point
 *   W : an indestructible wall
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
	MAP_CELL_CONTENT_EMPTY, //! No item or wall in the cell.
	MAP_CELL_CONTENT_WALL, //! There is an indestructible wall in the cell.
	MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE, //! The obstacle can be broken by a bomb.
	MAP_CELL_CONTENT_PLAYER_SPAWN_POINT, //! A player can spawn here.
} TMapCellContent;

/** A map cell. */
typedef struct
{
	TMapCellContent Content; //!< What is located in the cell.
	TGameTileID Tile_ID; //!< The corresponding tile.
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

// MapSpawnItem 

#endif