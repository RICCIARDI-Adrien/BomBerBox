/** @file Network.c
 * @see Network.h for description.
 * @author Adrien RICCIARDI
 */

#include <arpa/inet.h>
#include <Configuration.h>
#include <errno.h>
#include <netinet/in.h>
#include <Network.h>
#include <Player.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** The server socket. */
static int Network_Server_Socket;

/** The clients sockets. */
static int Network_Client_Sockets[CONFIGURATION_MAXIMUM_PLAYERS_COUNT];

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
int NetworkCreateServer(char *String_IP_Address, unsigned short Port)
{
	struct sockaddr_in Address;
	
	// Try to create the socket
	Network_Server_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (Network_Server_Socket == -1)
	{
		printf("[%s:%d] Error : failed to create the server socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Make the socket instantly reusable
	// TODO
	
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

int NetworkWaitForPlayers(int Expected_Players_Count, TPlayer *Pointer_Players)
{
	int i;
	unsigned char Command_Code;
	
	// Make sure there is enough room for all players
	if (Expected_Players_Count > CONFIGURATION_MAXIMUM_PLAYERS_COUNT)
	{
		printf("[%s:%d] Error : there are more expected players (%d) than the server can handle (%d).\n", __FUNCTION__, __LINE__, Expected_Players_Count, CONFIGURATION_MAXIMUM_PLAYERS_COUNT);
		return 1;
	}
	
	// Wait for the clients to connect
	for (i = 0; i < Expected_Players_Count; i++)
	{
		// Tell how many connections to wait for
		if (listen(Network_Server_Socket, Expected_Players_Count) == -1)
		{
			printf("[%s:%d] Error : listen() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
			return 1;
		}
		
		// Try to connect the client
		Network_Client_Sockets[i] = accept(Network_Server_Socket, NULL, NULL);
		if (Network_Client_Sockets[i] == -1)
		{
			printf("[%s:%d] Error : client #%d failed to connect (%s).\n", __FUNCTION__, __LINE__, i + 1, strerror(errno));
			i--; // Do as if nothing happened to avoid kicking the other clients by stopping the server
			continue;
		}
		
		// Retrieve the client name
		// Discard the command code
		if (read(Network_Client_Sockets[i], &Command_Code, 1) != 1)
		{
			printf("[%s:%d] Error : failed to read the player name command code (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
			return 1;
		}
		// Get the command payload
		if (read(Network_Client_Sockets[i], Pointer_Players[i].Name, CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH - 1) < 1)
		{
			printf("[%s:%d] Error : failed to read the player name (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
			return 1;
		}
		Pointer_Players[i].Name[CONFIGURATION_MAXIMUM_PLAYER_NAME_LENGTH - 1] = 0; // Force a terminating zero
		
		printf("Client #%d connected, name : %s.\n", i + 1, Pointer_Players[i].Name);
	}
	
	return 0;
}
