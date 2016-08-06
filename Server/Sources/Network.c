/** @file Network.c
 * @see Network.h for description.
 * @author Adrien RICCIARDI
 */

#include <arpa/inet.h>
#include <Configuration.h>
#include <errno.h>
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
	NETWORK_COMMAND_CONNECT_TO_SERVER //!< The client tries to connect to the server.
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

int NetworkSendCommandDrawTile(int Socket, int Tile_ID, int Row, int Column)
{
	unsigned char Temp_Byte;
	
	// Send the command
	Temp_Byte = NETWORK_COMMAND_DRAW_TILE;
	if (write(Socket, &Temp_Byte, 1) != 1)
	{
		printf("[%s:%d] Error : failed to send the 'draw tile' command (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Send the tile ID
	Temp_Byte = (unsigned char) Tile_ID;
	if (write(Socket, &Temp_Byte, 1) != 1)
	{
		printf("[%s:%d] Error : failed to send the tile ID (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Send the row coordinate
	Temp_Byte = (unsigned char) Row;
	if (write(Socket, &Temp_Byte, 1) != 1)
	{
		printf("[%s:%d] Error : failed to send the row coordinate (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Send the column coordinate
	Temp_Byte = (unsigned char) Column;
	if (write(Socket, &Temp_Byte, 1) != 1)
	{
		printf("[%s:%d] Error : failed to send the column coordinate (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	return 0;
}

int NetworkSendCommandDrawText(int Socket, char *String_Text)
{
	unsigned char Command = NETWORK_COMMAND_DRAW_TEXT;
	
	// Send the command
	if (write(Socket, &Command, 1) != 1)
	{
		printf("[%s:%d] Error : failed to send the 'draw text' command (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Send the payload
	if (write(Socket, String_Text, strlen(String_Text)) <= 0)
	{
		printf("[%s:%d] Error : failed to send the 'draw text' payload (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	return 0;
}