/** @file Map.c
 * @see Map.h for description.
 * @author Adrien RICCIARDI
 */

#include <Configuration.h>
#include <errno.h>
#include <fcntl.h>
#include <Map.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//-------------------------------------------------------------------------------------------------
// Public variables
//-------------------------------------------------------------------------------------------------
TMapCellContent Map[CONFIGURATION_MAP_ROWS_COUNT][CONFIGURATION_MAP_COLUMNS_COUNT];

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
int MapLoad(char *String_File_Path)
{
	int File_Descriptor, Row, Column;
	char Character;
	
	// Try to open the file
	File_Descriptor = open(String_File_Path, O_RDONLY);
	if (File_Descriptor == -1)
	{
		printf("[%s:%d] Error : could not open the map file (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	
	// Load the whole file content
	for (Row = 0; Row < CONFIGURATION_MAP_ROWS_COUNT; Row++)
	{
		for (Column = 0; Column < CONFIGURATION_MAP_COLUMNS_COUNT; Column++)
		{
			// Get the next character
			do
			{
				if (read(File_Descriptor, &Character, 1) != 1)
				{
					printf("[%s:%d] Error : could not read the cell (row : %d, column : %d) from the map file (%s).\n", __FUNCTION__, __LINE__, Row + 1, Column + 1, strerror(errno));
					close(File_Descriptor);
					return 1;
				}
			} while (Character == '\n'); // Bypass new line character
			
			// Is the character allowed ?
			switch (Character)
			{
				case MAP_CELL_CONTENT_EMPTY:
				case MAP_CELL_CONTENT_WALL:
				case MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE:
				case MAP_CELL_CONTENT_PLAYER_SPAWN_POINT:
				case MAP_CELL_CONTENT_ITEM_GHOST:
				case MAP_CELL_CONTENT_ITEM_SHIELD:
				case MAP_CELL_CONTENT_ITEM_POWER_UP:
					Map[Row][Column] = (TMapCellContent) Character;
					break;
					
				default:
					printf("[%s:%d] Error : map cell (row : %d, column : %d) value (%c) is not allowed.\n", __FUNCTION__, __LINE__, Row + 1, Column + 1, Character);
					close(File_Descriptor);
					return 1;
			}

		}
	}
	
	close(File_Descriptor);
	return 0;
}