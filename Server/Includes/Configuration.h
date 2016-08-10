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
#define CONFIGURATION_MAXIMUM_PLAYERS_COUNT 8

/** How long can be a player name. */
#define CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH 64

/** How high the map is. */
#define CONFIGURATION_MAP_ROWS_COUNT 15
/** How wide the map is. */
#define CONFIGURATION_MAP_COLUMNS_COUNT 20

/** Destructible obstacles generation probability in percent. */
#define CONFIGURATION_DESTRUCTIBLE_OBSTACLES_GENERATION_PERCENTAGE 35
/** The probability to spawn an item when a Destructible obstacle is broken. */
#define CONFIGURATION_DESTRUCTIBLE_OBSTACLE_ITEM_SPAWNING_PERCENTAGE 35

/** The maximum length of a 'draw text' command message. */
#define CONFIGURATION_COMMAND_DRAW_TEXT_MESSAGE_MAXIMUM_SIZE 255

/** A game tick duration (in nanoseconds). */
#define CONFIGURATION_GAME_TICK 50000000L

/** How long a bomb will remain before exploding (in tick units). */
#define CONFIGURATION_BOMB_EXPLOSION_TIMER (2000000000L / CONFIGURATION_GAME_TICK)
/** How long should an explosion tile be displayed (in tick unit). */
#define CONFIGURATION_BOMB_EXPLOSION_PROPAGATION_TIME (500000000L / CONFIGURATION_GAME_TICK)

#endif