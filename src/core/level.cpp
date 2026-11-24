#include <level.h>
#include <util.h>
#include<fstream>
#include<cstdlib>
#include<cstdio>
#include<cstring>
#include<cctype>

// Modular implementation using primitive arrays for map parsing and verification

// Room names, indexed[floor][room]
static const char* ROOM_NAMES[NUM_FLOORS][ROOMS_PER_FLOOR] = {{"Entrance lobby", "Recpetion", "Waiting Area", "utility Room"}, {"Lounge", "Archive", "Lab", "Meeting Room"}, {"Stairwell Annex", "Security Vesitbule", "Animal room", "Control Room"}, {"First Aid satation", "Medical Room", "Maintenance Coridor", "Office"}, {"security room", "Train-Boarding Platform", "Storage Room", "Observation Deck"}};


bool loadBuildingMap(const char* filename, char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1], char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM], int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR])
{
    for(int i = 0; i < NUM_FLOORS; i++)                 // Start with an empty building
    {
        for(int f = 0; f < ROOMS_PER_FLOOR - 1; f++)
        {
            horizontalConn[i][f] = CONN_NONE;
        }
        for(int g = 0; g < ROOMS_PER_FLOOR; g++)
        {
            entityCounts[i][g] = 0;
            for(int h = 0; h < MAX_ENTITIES_PER_ROOM; h++)
            {
                roomEntities[i][g][h] = ENT_NONE;
            }
        }
    }
}