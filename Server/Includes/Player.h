/** @file Player.h
 * All useful information to keep a player on the map.
 * @author Adrien RICCIARDI
 */

#ifndef H_PLAYER_H
#define H_PLAYER_H

#include <Configuration.h>

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** A player attributes. */
typedef struct
{
	char Name[CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH]; //!< The player name.
	int Row;
	int Column;
	// Bonus items
} TPlayer;

#endif
