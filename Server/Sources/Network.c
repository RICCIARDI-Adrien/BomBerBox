/** @file Network.c
 * @see Network.h for description.
 * @author Adrien RICCIARDI
 */

#include <arpa/inet.h>
#include <Configuration.h>
#include <errno.h>
#include <Game.h>
#include <netinet/in.h>
#include <Network.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
/** All available commands. */
typedef enum
{
	NETWORK_COMMAND_DRAW_TILE, //!< The client must draw a tile at the specified location.
	NETWORK_COMMAND_DRAW_TEXT, //!< The client must draw a string at the dedicated location.
	NETWORK_COMMAND_CONNECT_TO_SERVER, //!< The client tries to connect to the server.
	NETWORK_COMMAND_GET_EVENT //!< The client sends a button event to the server.
} TNetworkCommand;

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** The server socket. */
static int Network_Server_Socket;

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
int NetworkCreateServer(char *String_IP_Address, unsigned short Port)
{
	struct sockaddr_in Address;
	int Option_Value = 1;
	
	// Try to create the socket
	Network_Server_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (Network_Server_Socket == -1)
	{
		printf("[%s:%d] Error : failed to create the server socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Make the socket instantly reusable
	if (setsockopt(Network_Server_Socket, SOL_SOCKET, SO_REUSEADDR, &Option_Value, sizeof(Option_Value)) == -1)
	{
		printf("[%s:%d] Error : failed to set the server socket as reusable (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Try to bind the server
	Address.sin_family = AF_INET;
	Address.sin_port = htons(Port);
	Address.sin_addr.s_addr = inet_addr(String_IP_Address);
	if (bind(Network_Server_Socket, (const struct sockaddr *) &Address, sizeof(Address)) == -1)
	{
		printf("[%s:%d] Error : failed to bind the server (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		close(Network_Server_Socket);
		return 1;
	}
	
	return 0;
}

int NetworkWaitForPlayerConnection(int *Pointer_Player_Socket, char *Pointer_Player_Name)
{
	unsigned char Command_Code;
	
	// Tell how many connections to wait for
	if (listen(Network_Server_Socket, CONFIGURATION_MAXIMUM_PLAYERS_COUNT) == -1)
	{
		printf("[%s:%d] Error : listen() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Try to connect the client
	*Pointer_Player_Socket = accept(Network_Server_Socket, NULL, NULL);
	if (*Pointer_Player_Socket == -1)
	{
		printf("[%s:%d] Error : connect() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Retrieve the client name
	// Wait for the command code
	do
	{
		if (read(*Pointer_Player_Socket, &Command_Code, 1) != 1)
		{
			printf("[%s:%d] Error : failed to read the player name command code (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
			return 1;
		}
	} while (Command_Code != NETWORK_COMMAND_CONNECT_TO_SERVER);
	
	// Get the command payload
	if (read(*Pointer_Player_Socket, Pointer_Player_Name, CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH - 1) < 1)
	{
		printf("[%s:%d] Error : failed to read the player name (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	Pointer_Player_Name[CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH - 1] = 0; // Force a terminating zero
		
	return 0;
}

int NetworkGetEvent(TGamePlayer *Pointer_Player, TNetworkEvent *Pointer_Event)
{
	int Events_Count;
	fd_set File_Descriptors_Set;
	struct timeval Select_Timeout;
	unsigned char Command_Data[2];
	
	// Ignore dead players
	if (!Pointer_Player->Is_Alive)
	{
		*Pointer_Event = NETWORK_EVENT_NONE;
		return 0;
	}
	
	// Create the set of file descriptors (it must created for each call)
	FD_ZERO(&File_Descriptors_Set);
	FD_SET(Pointer_Player->Socket, &File_Descriptors_Set);
		
	// Set the timeout value to zero to make select() instantly return (it must be reset each time too)
	Select_Timeout.tv_sec = 0;
	Select_Timeout.tv_usec = 0;
	
	// Use select() as it can poll a blocking socket without blocking the program
	Events_Count = select(Pointer_Player->Socket + 1, &File_Descriptors_Set, NULL, NULL, &Select_Timeout);
	if (Events_Count == -1)
	{
		printf("[%s:%d] Error : select() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Did the client sent some event ?
	if (Events_Count == 0)
	{
		*Pointer_Event = NETWORK_EVENT_NONE;
		return 0;
	}
	
	// TODO check for player deconnection
	
	// Get the command
	if (read(Pointer_Player->Socket, Command_Data, sizeof(Command_Data)) != sizeof(Command_Data))
	{
		if (errno == 0) // The player deconnected
		{
			close(Pointer_Player->Socket);
			Pointer_Player->Socket = -1;
			return 2;
		}
		
		printf("[%s:%d] Error : failed to receive the 'get event' command (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	// Retrieve the event content
	*Pointer_Event = Command_Data[1];
	
	return 0;
}

int NetworkSendCommandDrawTile(TGamePlayer *Pointer_Player, int Tile_ID, int Row, int Column)
{
	unsigned char Command_Data[4];
	
	// Ignore deconnected players
	if (Pointer_Player->Socket == -1) return 0;
	
	// Prepare the command
	Command_Data[0] = NETWORK_COMMAND_DRAW_TILE;
	Command_Data[1] = (unsigned char) Tile_ID;
	Command_Data[2] = (unsigned char) Row;
	Command_Data[3] = (unsigned char) Column;
	
	// Send the command
	if (write(Pointer_Player->Socket, Command_Data, sizeof(Command_Data)) != sizeof(Command_Data))
	{
		printf("[%s:%d] Error : failed to send the 'draw tile' command (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	return 0;
}

int NetworkSendCommandDrawText(TGamePlayer *Pointer_Player, char *String_Text)
{
	unsigned char Command_Data[2 + CONFIGURATION_COMMAND_DRAW_TEXT_MESSAGE_MAXIMUM_SIZE];
	int Text_Size, Command_Size;
	
	// Ignore deconnected players
	if (Pointer_Player->Socket == -1) return 0;
	
	// Prepare the command
	Command_Data[0] = NETWORK_COMMAND_DRAW_TEXT;
	Text_Size = strnlen(String_Text, CONFIGURATION_COMMAND_DRAW_TEXT_MESSAGE_MAXIMUM_SIZE);
	Command_Data[1] = (unsigned char) Text_Size;
	memcpy(&Command_Data[2], String_Text, Text_Size);
	
	// Send the command
	Command_Size = 2 + Text_Size; // Compute the command total size in bytes
	if (write(Pointer_Player->Socket, Command_Data, Command_Size) != Command_Size)
	{
		printf("[%s:%d] Error : failed to send the 'draw text' command (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	return 0;
}