/** @file Main.c
 * BomBerBox server entry point and main loop.
 * @author Adrien RICCIARDI
 */

#include <Configuration.h>
#include <Game.h>
#include <Map.h>
#include <Network.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//-------------------------------------------------------------------------------------------------
// Entry point
//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	char *String_IP_Address, *String_Map_File;
	unsigned short Port;
	int Expected_Players_Count, Map_Spawn_Points_Count;
	
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
	
	// Initialize random numbers generator
	srand(time(NULL));
	
	// Try to load the map
	if (MapLoad(String_Map_File) != 0)
	{
		printf("[%s:%d] Error : failed to load the map.\n", __FUNCTION__, __LINE__);
		return EXIT_FAILURE;
	}
	
	// Are there enough spawn points for all players ?
	Map_Spawn_Points_Count = MapGetSpawnPointsCount();
	if (Map_Spawn_Points_Count < Expected_Players_Count)
	{
		printf("[%s:%d] Error : the map has only %d spawn points while %d players are expected.\n", __FUNCTION__, __LINE__, Map_Spawn_Points_Count, Expected_Players_Count);
		return EXIT_FAILURE;
	}
	printf("Map successfully loaded.\n");
	
	// Create the server
	if (NetworkCreateServer(String_IP_Address, Port) != 0)
	{
		printf("[%s:%d] Error : could not create the server on IP %s and port %u.\n", __FUNCTION__, __LINE__, String_IP_Address, Port);
		return EXIT_FAILURE;
	}
	printf("Server up. Waiting for clients.\n");

	// Start the game
	GameLoop(Expected_Players_Count);
	
	return EXIT_SUCCESS;
}