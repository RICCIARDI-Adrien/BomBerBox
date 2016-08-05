/** @file Main.c
 * BomBerBox server entry point and main loop.
 * @author Adrien RICCIARDI
 */

#include <Configuration.h>
#include <Map.h>
#include <Network.h>
#include <Player.h>
#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** All players. */
static TPlayer Players[CONFIGURATION_MAXIMUM_PLAYERS_COUNT];

//-------------------------------------------------------------------------------------------------
// Entry point
//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	char *String_IP_Address, *String_Map_File;
	unsigned short Port;
	int Expected_Players_Count;
	
	// Check parameters
	if (argc != 5)
	{
		printf("Usage : %s IP_Address Port Map_File Expected_Players_Count\n", argv[0]);
		return EXIT_FAILURE;
	}
	String_IP_Address = argv[1];
	Port = atoi(argv[2]);
	String_Map_File = argv[3];
	Expected_Players_Count = atoi(argv[4]);
	if ((Expected_Players_Count <= 1) || (Expected_Players_Count > CONFIGURATION_MAXIMUM_PLAYERS_COUNT))
	{
		printf("[%s:%d] Error : minimum expected players are 2, maximum expected players are %d.\n", __FUNCTION__, __LINE__, CONFIGURATION_MAXIMUM_PLAYERS_COUNT);
		return EXIT_FAILURE;
	}
	
	// Try to load the map
	if (MapLoad(String_Map_File) != 0)
	{
		printf("[%s:%d] Error : failed to load the map.\n", __FUNCTION__, __LINE__);
		return EXIT_FAILURE;
	}
	printf("Map successfully loaded.\n");
	
	// Create the server
	if (NetworkCreateServer(String_IP_Address, Port) != 0)
	{
		printf("[%s:%d] Error : could not create the server on IP %s and port %u.\n", __FUNCTION__, __LINE__, String_IP_Address, Port);
		return EXIT_FAILURE;
	}
	printf("Server up.\n");
	
	// Wait for the clients to connect
	if (NetworkWaitForPlayers(Expected_Players_Count, Players) != 0) return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}