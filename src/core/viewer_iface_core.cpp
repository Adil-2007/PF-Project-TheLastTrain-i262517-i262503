#include "viewer_iface.h"
#include "util.h"
#include "level.cpp"
#include <iostream>
using namespace std;

// Terminal UI and dashboard rendering using primitive parameters

static void printBar(int value, int maxValue)
{
    const int WIDTH = 20;
    int filled = (maxValue > 0) ? (value * WIDTH) / maxValue : 0;
    cout << "[";
    for(int i = 0; i < WIDTH; i++)
        cout << (i < filled ? '#' : '-');
    cout << "]";
}

void renderDashboard(int playerFloor, int playerRoom, int energy, int time, int score, const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount, int emergencyTripsLeft)
{
    cout << "---------------------------------------------\n";
    cout << " Location : F" << playerFloor << "-R" << playerRoom << " (" << getRoomName(playerFloor, playerRoom) << ")\n";
    cout << " Energy : ";
    printBar(energy, MAX_ENERGY);
    cout << " " << energy << "/" << MAX_ENERGY << "\n";
    cout << "Time : ";
    printBar(time, MAX_TIME);
    cout << " " << time << "/" << MAX_TIME << "\n";
    cout << "Score : " << score << " / 520\n";
    cout << "Inventory : ";
    if(inventoryCount == 0)
    {
        cout << "(empty)";
    }
    for(int i = 0; i < inventoryCount; i++)
    {
        if(i > 0)
            cout << ", ";
        cout << "[" << inventory[i] << "]" << getEntityName(inventory[i]);
    }

    cout << "\n";
    cout << " Lift 2 emergency Trips left: " << emergencyTripsLeft << "\n";
    cout << "---------------------------------------------\n";
}

void renderRoomDetails(int floor, int room, const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM], int entityCount)
{
    cout << "You sarch F" << floor << "-R" << room << " (" << getRoomName(floor, room) << ").\n";
    if(entityCount == 0)
    {
        cout << " Nothing useful here.\n";
        return;
    }
    for(int e = 0; e < entityCount; e++)
    {
        char c = roomEntities[floor - 1][room - 1][e];
        cout << " [" << c << "] " << getEntityName(c) << "\n";
    }
}

// Separator between rooms on each floor (index[floor-1][gap]; '=' marks the Lift 1 shaft)
static const char MAP_CONN[NUM_FLOORS][ROOMS_PER_FLOOR - 1] = {
    {'W', '=', 'D'} // Floor 1
    , {'D', '=', 'D'} // Floor 2
    , {'L','W','D'} // Floor 3
    , {'W', '#', 'D'} // Floor 4
    , {'D', 'D', 'W'} // Floor 5
};

static const char* MAP_LEFT[NUM_FLOORS] = {
    "S1 ", "S1 S2 ", "S2 ", "S3 ", "S3 "
};

static const char* MAP_RIGHT[NUM_FLOORS] = {
    ""," L2", " (L2 bypasses F3)", "L2", "L2"
};

void renderAsciiBuilding(int playerFloor, int playerRoom)
{
    const char* border = "        +-------------+-------------+-------------+-------------+\n";

    cout << "\n The LAST TRAIN - FACILITY MAP\n";
    cout << border;
    
    for(int f = NUM_FLOORS; f >= 1; f--)
    {
        cout << " " << MAP_LEFT[f - 1] << "|";      // Line1: Room code and connectors
        for(int r = 1; r <= ROOMS_PER_FLOOR; r++)
        {
            cout << " F" << f << "-R" << r << " ";
            if(r < ROOMS_PER_FLOOR)
                cout << MAP_CONN[f - 1][r - 1];
        }
        cout << "|" << MAP_RIGHT[f - 1] << "\n";

        cout << " |";       // Line2: Player marker
        for(int r = 1; r <= ROOMS_PER_FLOOR; r++)
        {
            if(f == playerFloor && r == playerRoom)
                cout << " <YOU> ";
            else
                cout << " ";
            if(r < ROOMS_PER_FLOOR)
                cout << "|";
        }
        cout << "|\n";
        cout << border;
    }
    cout << " D = Door  W = Window  L = Locked door  # = Wall   == Lift 1 (F1/F2, R2-R3)\n";
    cout << " S1: F1-R1 <-> F2-R1   S2: F2-R1 <-> F3-R1     S3: F4-R1 <-> F5-R1     L2: F2-R4/F4-R4/F5-R4\n\n";
}

void renderObjectivesChecklist(const bool objectives [NUM_OBJECTIVES])
{
    const int COUNT = 12;
    const int idx[COUNT] = {
        OBJ_MASTER_KEY, OBJ_CLUE_4, OBJ_CLUE_2, OBJ_CLUE_7, OBJ_CABINET_UNLOCKED,OBJ_TRAIN_KEY, OBJ_PASSENGER_RESCUED, OBJ_CAT_SECURED, OBJ_FUSE_INSTALLED, OBJ_LIFT_PASS, OBJ_TRAIN_PREPARED, OBJ_MISSION_WON
    };
    const char* label[COUNT] = {
        "Master Key collected   (+10)", "Clue digit 4 found     (+10)", "Clue digit 2 found     (+10)", "Clue digit 7 found     (+10)", "Cabinet unlocked (427)     (+30)", "Train Key acquired     (+10)", "Passenger rescued      (+100)", "Cat secured       (+100)", "Fuse installed at panel       (+20)", "Lift Pass collected        (+20)", "Train prepared & boarded       (+100)", "Full mission victory      (+100)"
    };

    printHeader("OBJECTIVES");
    for(int i = 0; i < COUNT; i++)
    {
        cout << " [" << (objectives[idx[i]] ? "X" : " ") << "] " << label[i] << "\n";
    }

    cout << " Lift 2 status: " << (objectives[OBJ_LIFT2_ACTIVE] ? "OPERATIONAL" : "not repaired") << "\n";
}