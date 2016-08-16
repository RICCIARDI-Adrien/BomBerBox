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
	char *String_IP_Address;
	unsigned short Port;
	int Expected_Players_Count;
	
	// Check parameters
	if (argc != 4)
	{
		printf("Usage : %s IP_Address Port Expected_Players_Count\n", argv[0]);
		return EXIT_FAILURE;
	}
	String_IP_Address = argv[1];
	Port = atoi(argv[2]);
	Expected_Players_Count = atoi(argv[3]);
	if ((Expected_Players_Count <= 1) || (Expected_Players_Count > CONFIGURATION_MAXIMUM_PLAYERS_COUNT))
	{
		printf("[%s:%d] Error : minimum expected players are 2, maximum expected players are %d.\n", __FUNCTION__, __LINE__, CONFIGURATION_MAXIMUM_PLAYERS_COUNT);
		return EXIT_FAILURE;
	}
	
	// Initialize random numbers generator
	srand(time(NULL));
	
	// Create the server
	if (NetworkCreateServer(String_IP_Address, Port) != 0)
	{
		printf("[%s:%d] Error : could not create the server on IP %s and port %u.\n", __FUNCTION__, __LINE__, String_IP_Address, Port);
		return EXIT_FAILURE;
	}
	printf("Server up. Waiting for clients.\n");

	// Run the game forever
	while (1)
	{
		if (GameLoop(Expected_Players_Count) != 0)
		{
			// Shutdown server
			// TODO
			
			return EXIT_FAILURE;
		}
	}
}