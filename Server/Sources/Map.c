/** @file Map.c
 * @see Map.h for description.
 * @author Adrien RICCIARDI
 */

#include <Configuration.h>
#include <errno.h>
#include <fcntl.h>
#include <Map.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** How many maps are available. */
#define MAPS_COUNT (sizeof(String_Maps_File_Names) / sizeof(char *))

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
/** A cell coordinates in the map. */
typedef struct
{
	int Row;
	int Column;
} TMapCellCoordinate;

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** How many spawn points the last loaded map have. */
static int Map_Spawn_Points_Count;
/** The spawn points location. */
static TMapCellCoordinate Map_Spawn_Points_Coordinates[CONFIGURATION_MAXIMUM_PLAYERS_COUNT];

/** All available maps file name. */
static char *String_Maps_File_Names[] = { CONFIGURATION_MAP_FILE_NAMES };

//-------------------------------------------------------------------------------------------------
// Public variables
//-------------------------------------------------------------------------------------------------
TMapCell Map[CONFIGURATION_MAP_ROWS_COUNT][CONFIGURATION_MAP_COLUMNS_COUNT];

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
/** Load a map from a text file.
 * @param String_File_Path The file location.
 * @return 0 if the map was successfully loaded,
 * @return 1 if an error occurred.
 */
static inline int MapLoad(char *String_File_Path)
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
	
	Map_Spawn_Points_Count = 0;
	
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
			
			// Reset the map cell
			Map[Row][Column].Content = MAP_CELL_CONTENT_EMPTY;
			Map[Row][Column].Tile_ID = GAME_TILE_ID_EMPTY;
			Map[Row][Column].Explosion_State = MAP_EXPLOSION_STATE_NO_BOMB;
			
			// Is the character allowed ?
			switch (Character)
			{
				case ' ':
					// Generate or not a destructible object in this empty cell
					if (rand() % 100 < CONFIGURATION_DESTRUCTIBLE_OBSTACLES_GENERATION_PERCENTAGE)
					{
						Map[Row][Column].Content = MAP_CELL_CONTENT_DESTRUCTIBLE_OBSTACLE;
						Map[Row][Column].Tile_ID = GAME_TILE_ID_DESTRUCTIBLE_OBSTACLE;
					}
					else
					{
						Map[Row][Column].Content = MAP_CELL_CONTENT_EMPTY;
						Map[Row][Column].Tile_ID = GAME_TILE_ID_EMPTY;
					}
					break;
					
				case 'W':
					Map[Row][Column].Content = MAP_CELL_CONTENT_WALL;
					Map[Row][Column].Tile_ID = GAME_TILE_ID_WALL;
					break;
					
				case 'S':
					Map[Row][Column].Content = MAP_CELL_CONTENT_PLAYER_SPAWN_POINT;
					Map[Row][Column].Tile_ID = GAME_TILE_ID_EMPTY;
					
					// Store the spawn point coordinates
					Map_Spawn_Points_Coordinates[Map_Spawn_Points_Count].Row = Row;
					Map_Spawn_Points_Coordinates[Map_Spawn_Points_Count].Column = Column;
					Map_Spawn_Points_Count++;
					break;
					
				case 'N':
					Map[Row][Column].Content = MAP_CELL_CONTENT_NO_DESTRUCTIBLE_OBSTACLE_ZONE;
					Map[Row][Column].Tile_ID = GAME_TILE_ID_EMPTY;
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

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
int MapLoadRandom(void)
{
	char *String_Map_File_Name;
	char String_Map_Full_File_Path[256];
	
	// Choose a random map
	String_Map_File_Name = String_Maps_File_Names[rand() % MAPS_COUNT];
	
	// Create the map file path to load
	snprintf(String_Map_Full_File_Path, sizeof(String_Map_Full_File_Path), "%s/%s", CONFIGURATION_MAPS_PATH, String_Map_File_Name);
	printf("Loading map %s...\n", String_Map_Full_File_Path);
	
	return MapLoad(String_Map_Full_File_Path);
}

int MapGetSpawnPointsCount(void)
{
	return Map_Spawn_Points_Count;
}

void MapGetSpawnPointCoordinates(int Spawn_Point_Index, int *Pointer_Row, int *Pointer_Column)
{
	// Make sure the spawn point is existing
	if (Spawn_Point_Index > Map_Spawn_Points_Count)
	{
		*Pointer_Row = 0;
		*Pointer_Column = 0;
		return;
	}
	
	*Pointer_Row = Map_Spawn_Points_Coordinates[Spawn_Point_Index].Row;
	*Pointer_Column = Map_Spawn_Points_Coordinates[Spawn_Point_Index].Column;
}

void MapSpawnItem(int Row, int Column)
{
	TMapCell *Pointer_Cell;
	
	// Cache cell address
	Pointer_Cell = &Map[Row][Column];
	
	// Choose whether an item will spawn or not
	if (rand() % 100 > CONFIGURATION_DESTRUCTIBLE_OBSTACLE_ITEM_SPAWNING_PERCENTAGE)
	{
		Pointer_Cell->Content = MAP_CELL_CONTENT_EMPTY;
		Pointer_Cell->Tile_ID = GAME_TILE_ID_EMPTY;
		return;
	}
	
	// Select which item to spawn
	switch (rand() % 4)
	{
		case 0:
			Pointer_Cell->Content = MAP_CELL_CONTENT_ITEM_SHIELD;
			Pointer_Cell->Tile_ID = GAME_TILE_ITEM_SHIELD;
			break;
			
		case 1:
			Pointer_Cell->Content = MAP_CELL_CONTENT_ITEM_POWER_UP_BOMB_RANGE;
			Pointer_Cell->Tile_ID = GAME_TILE_ITEM_POWER_UP_BOMB_RANGE;
			break;
			
		case 2:
			Pointer_Cell->Content = MAP_CELL_CONTENT_ITEM_POWER_UP_BOMBS_COUNT;
			Pointer_Cell->Tile_ID = GAME_TILE_ITEM_POWER_UP_BOMBS_COUNT;
			break;
			
		case 3:
			GameDropBomb(Row, Column, (rand() % 3) + 2, NULL);
			Pointer_Cell->Tile_ID = GAME_TILE_BOMB;
			break;
	}
}
	