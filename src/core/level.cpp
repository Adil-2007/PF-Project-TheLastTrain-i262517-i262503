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

    std::ifstream fin(filename);
    if(!fin) 
        return false;

    char line[MAX_LINE_LEN];
    char entLine[MAX_LINE_LEN];
    int floorsFound = 0;

    while(fin.getline(line, MAX_LINE_LEN))
    {
        int len = (int)strlen(line);

        int pos = -1;           // Look for the token "F<digit>-R1" that starts a floor row
        int floor = 0;
        for(int i = 0; i + 4 < len; i++)
        {
            if(line[i] == 'F' && isdigit((unsigned char)line[i + 1]) && line[i + 2] == '-' && line[i + 3] == 'R' && line[i + 4] == '1')
            {
                pos = i;
                floor = line[i + 1] - '0';
                break;
            }
        }

        if(pos < 2 || line[pos - 2] != '|' || floor < 1 || floor > NUM_FLOORS)
            continue;

        int base = pos - 2;        // column of the left wall '|'

        // 1)) Horizontal connections between neighbouring rooms

        for(int g = 0; g < ROOMS_PER_FLOOR - 1; g++)
        {
            int col = base + 12 * (g + 1);
            char c;
            if(col < len)
                c = line[col];
            else
                c = ' ';
            
                if(c == CONN_DOOR || c == CONN_WINDOW || c == CONN_LOCKED_DOOR || c == CONN_SOLID_WALL)
                    horizontalConn[floor - 1][g] = c;
        }

        // 2)) Entity line right below the name line
        if(!fin.getline(entLine, MAX_LINE_LEN))
            break;
        int elen = (int)strlen(entLine);

        for(int r = 0; r < ROOMS_PER_FLOOR; r++)
        {
            int startCol = base + 1 + 12 * r;
            int endCol = base + 11 + 12 * r;
            for(int col = startCol; col <= endCol && col < elen; col++)
            {
                char c = entLine[col];
                if(c != ' ' && c != '|' && entityCounts[floor - 1][r] < MAX_ENTITIES_PER_ROOM)
                {
                    roomEntities[floor - 1][r][entityCounts[floor - 1][r]] = c;
                    entityCounts[floor - 1][r]++;
                }
            }
        }
        floorsFound++;
    }

    // F1-R4 holds TWO energy packs, the map draws only one symbol

    int packs = 0;
    for(int e = 0; e < entityCounts[0][3]; e++)
    {
        if(roomEntities[0][3][e] == ENT_ENERGY_PACK)
            packs++;
    }
    if(packs == 1 && entityCounts[0][3] << MAX_ENTITIES_PER_ROOM)
    {
        roomEntities[0][3][entityCounts[0][3]] = ENT_ENERGY_PACK;
        entityCounts[0][3]++;
    }

    return floorsFound == NUM_FLOORS;
}

// Turns "Yes/No" into true/false

static bool isYes(const char* value)
{
    return stringEquals(value, "YES") || stringEquals(value, "TRUE") || stringEquals(value, "1");
}

bool loadInitialState(const char* filename, int& playerFloor, int& playerRoom, int& energy, int& time, int& score, char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount, bool objectives[NUM_OBJECTIVES], int& emergencyTripsLeft)
{
    // Copied values from config.h
    playerFloor = 0;
    playerRoom = 0;
    energy = INITIAL_ENERGY;
    time = INITIAL_TIME;
    score = INITIAL_SCORE;
    inventoryCount = 0;
    emergencyTripsLeft = 0;

    for(int i = 0; i < MAX_INVENTORY_CAPACITY; i++)
        inventory[i] = ENT_NONE;
    
    for(int i = 0; i < NUM_OBJECTIVES; i++)
        objectives[i] = false;

    std::ifstream fin(filename);
    if(!fin) 
        return false;

    char line[MAX_LINE_LEN];
    while(fin.getline(line, MAX_LINE_LEN))
    {
        char key[64];
        char value[64];
        if(sscanf(line, " %63[^:] %63s", key, value) != 2)
            continue;
        trimWhitespace(key);

        if(stringEquals(key, "START_ROOM"))
        {
            int f = 0, r = 0;
            if(parseRoomCode(value, f, r))
            {
                playerFloor = f;
                playerRoom = r;
            }
        }
        else if (stringEquals(key, "INITIAL_ENERGY"))
        {
            energy = atoi(value);
        }
        else if(stringEquals(key, "INITIAL_TIME"))
        {
            time = atoi(value);
        }
        else if(stringEquals(key, "INITIAL_SCORE"))
        {
            score = atoi(value);
        }
        else if(stringEquals(key, "INITIAL_INVENTORY"))
        {
            if(!stringEquals(value, "NONE"))
            {
                for(int i = 0; value[i] != '\0' && inventoryCount < MAX_INVENTORY_CAPACITY; i++)
                {
                    if(value[i] != ',')
                    {
                        inventory[inventoryCount] = value[i];
                        inventoryCount++;
                    }
                }
            }
        }
        else if (stringEquals(key, "CLUES_COLLECTED"))
        {
            if(!stringEquals(value, "NONE"))
            {
                for(int i = 0; value[i] != '\0'; i++)
                {
                    if(value[i] == ENT_CLUE_4)
                        objectives[OBJ_CLUE_4] = true;
                    if(value[i] == ENT_CLUE_2)
                        objectives[OBJ_CLUE_4] = true;
                    if(value[i] == ENT_CLUE_7)
                        objectives[OBJ_CLUE_4] = true;
                }
            }
        }
        else if(stringEquals(key, "PASSENGER_RESCUED"))
        {
            objectives[OBJ_PASSENGER_RESCUED] = isYes(value);
        }
        else if(stringEquals(key, "CAT_RESCUED"))
        {
            objectives[OBJ_CAT_SECURED] = isYes(value);
        }
        else if(stringEquals(key, "CABINET_UNLOCKED"))
        {
            objectives[OBJ_CABINET_UNLOCKED] = isYes(value);
        }
        else if(stringEquals(key, "TRAIN_KEY_UNLOCKED"))
        {
            objectives[OBJ_TRAIN_KEY] = isYes(value);
        }
        else if(stringEquals(key, "FUSE_INSTALLED"))
        {
            objectives[OBJ_FUSE_COLLECTED] = isYes(value);
        }
        else if(stringEquals(key, "LIFT_PASS_COLLECTED"))
        {
            objectives[OBJ_LIFT_PASS] = isYes(value);
        }
        else if(stringEquals(key, "TRAIN_PREPARED"))
        {
            objectives[OBJ_TRAIN_PREPARED] = isYes(value);
        }
    }

    energy = clampInt(energy, 0, MAX_ENERGY);
    time = clampInt(time, 0, MAX_TIME);
    return true;
}

bool validateStartingRoom(int startFloor, int startRoom, const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1])
{
    (void)horizontalConn;       // Every room in this building has at least one exit

    if(startFloor < 1 || startFloor > NUM_FLOORS)
        return false;
    if(startRoom < 1 || startRoom > ROOMS_PER_FLOOR)
        return false;
    if(startFloor == 5 && startRoom == 2)
        return false;
}

const char* getEntityName(char entitySymbol)
{
    switch(entitySymbol)
    {
        case ENT_MASTER_KEY:
            return "Master Key";
        case ENT_ENERGY_PACK:
            return "Energy Pack";
        case ENT_EMERGENCY_TOOLKIT:
            return "Emergency Toolkit";
        case ENT_CLUE_4:
            return "Clue Digit 4";
        case ENT_FUSE:
            return "Fuse";
        case ENT_CAT:
            return "Cat";
        case ENT_CONTROL_PANEL:
            return "Control Panel";
        case ENT_LIFT_PASS:
            return "Lift Pass";
        case ENT_PASSENGER:
            return "Injured Passenger";
        case ENT_MEDICAL_KIT:
            return "Medical Kit";
        case ENT_EMERGENCY_CARD:
            return "Emergency Access Card";
        case ENT_CLUE_2:
            return "Clue Digit 2";
        case ENT_CLUE_7:
            return "Clue Digit 7";
        case ENT_CABINET_TRAIN_KEY:
            return "Cabinet / Train key";
        case ENT_CAT_CARRIER:
            return "Cat Carrier";
        case ENT_OBSERVATION_TOKEN:
            return "Observation Token";
        default:
            return "Unknown Item";
    }
}

const char* getRoomName(int floor, int room)
{
    if(floor < 1 || floor > NUM_FLOORS || room < 1 || room > ROOMS_PER_FLOOR)
        return "Unknown Room";
    return ROOM_NAMES[floor - 1][room - 1];
}