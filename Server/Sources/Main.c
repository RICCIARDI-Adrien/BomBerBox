/** @file Main.c
 * BomBerBox server entry point and main loop.
 * @author Adrien RICCIARDI
 */

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
	
	// Check parameters
	if (argc != 3)
	{
		printf("Usage : %s IP_Address Port\n", argv[0]);
		return EXIT_FAILURE;
	}
	String_IP_Address = argv[1];
	Port = atoi(argv[2]);
	
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
		if (GameLoop() != 0)
		{
			// Shutdown server
			// TODO
			
			return EXIT_FAILURE;
		}
	}
}