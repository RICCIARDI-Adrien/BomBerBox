/** @file Network.h
 * Handle the communication protocol between the server and the clients.
 * @author Adrien RICCIARDI
 */

#ifndef H_NETWORK_H
#define H_NETWORK_H

#include <Game.h>

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** All events that can be received from a client. */
typedef enum
{
	NETWORK_EVENT_GO_UP, //!< The client pressed the "up" button.
	NETWORK_EVENT_GO_DOWN, //!< The client pressed the "down" button.
	NETWORK_EVENT_GO_LEFT, //!< The client pressed the "left" button.
	NETWORK_EVENT_GO_RIGHT, //!< The client pressed the "right" button.
	NETWORK_EVENT_DROP_BOMB, //!< The client pressed the "drop bomb" button.
	NETWORK_EVENT_DISCONNECT, //!< The client exited.
	NETWORK_EVENT_NONE //!< No event was sent by the client.
} TNetworkEvent;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Bind the game server on the requested IP address and port.
 * @param String_IP_Address The IP address to bind on.
 * @param Port The port to bind on.
 * @return 0 if the server was successfully created,
 * @return 1 if an error occurred.
 */
int NetworkCreateServer(char *String_IP_Address, unsigned short Port);

/** Check if a player is trying to connect or not.
 * @param Pointer_Player_Socket If a player connected, will contain the player socket on ouput. If no player connected, will be set to -1.
 * @param String_Player_Name If a player connected, will contain the name of the player. The string must be CONFIGURATION_MAXIMUM_PLAYERS_COUNT bytes long.
 * @return 0 on success,
 * @return 1 if an error occurred.
 */
int NetworkCheckPlayerConnection(int *Pointer_Player_Socket, char *String_Player_Name);

void NetworkShutdownServer(void);

/** Tell if the specified client sent an event or not.
 * @param Pointer_Player The player to get event from.
 * @param Pointer_Event On output, contain the event received from the client.
 * @return 0 on success,
 * @return 1 if an error occurred.
 */
int NetworkGetEvent(TGamePlayer *Pointer_Player, TNetworkEvent *Pointer_Event);

/** Tell the client to draw a specific tile at the specified coordinates.
 * @param Pointer_Player The player to send command to.
 * @param Tile_ID The tile the client must display.
 * @param Row The tile Y coordinate.
 * @param Column The tile X coordinate.
 * @return 0 on success,
 * @return 1 if an error occurred.
 */
int NetworkSendCommandDrawTile(TGamePlayer *Pointer_Player, int Tile_ID, int Row, int Column);

/** Send a displayable message to a client.
 * @param Pointer_Player The player to send command to.
 * @param String_Text The message the client must display.
 * @return 0 on success,
 * @return 1 if an error occurred.
 */
int NetworkSendCommandDrawText(TGamePlayer *Pointer_Player, char *String_Text);

#endif