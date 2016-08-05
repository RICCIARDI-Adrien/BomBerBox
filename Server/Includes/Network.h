/** @file Network.h
 * Handle the communication protocol between the server and the clients.
 * @author Adrien RICCIARDI
 */

#ifndef H_NETWORK_H
#define H_NETWORK_H

#include <Player.h>

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
	NETWORK_EVENT_DISCONNECT //!< The client exited.
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

/** Wait for the requested amount of players.
 * @param Expected_Players_Count How many players to wait for.
 * @param Pointer_Player_Names On output, contain the name of all connected players. The array must have CONFIGURATION_MAXIMUM_PLAYERS_COUNT entries.
 * @return 0 on success,
 * @return 1 if an error occurred.
 */ 
int NetworkWaitForPlayers(int Expected_Players_Count, TPlayer *Pointer_Players);

void NetworkShutdownServer(void);

int NetworkGetNextEvent(int *Pointer_Player_ID, TNetworkEvent *Pointer_Event);

int NetworkSendCommandDrawSprite(int Player_ID, int Sprite_ID, int Row, int Column);
int NetworkSendCommandDrawText(int Player_ID, char *String_Text);

#endif