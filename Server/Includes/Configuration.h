/** @file Configuration.h
 * Gather all program configurable parameters.
 * @author Adrien RICCIARDI
 */

#ifndef H_CONFIGURATION_H
#define H_CONFIGURATION_H

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
/** How many players can play in the same time. */
#define CONFIGURATION_MAXIMUM_PLAYERS_COUNT 4

/** How long can be a player name. */
#define CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH 64

/** How high the map is. */
#define CONFIGURATION_MAP_ROWS_COUNT 15
/** How wide the map is. */
#define CONFIGURATION_MAP_COLUMNS_COUNT 20

/** Destructible obstacles generation probability (see the code for the meaning of the value). */
#define CONFIGURATION_DESTRUCTIBLE_OBSTACLES_GENERATION_PROBABILITY 35

#endif