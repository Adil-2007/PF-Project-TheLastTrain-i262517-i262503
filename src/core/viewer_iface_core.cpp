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

static const char MAP_CONN[NUM_FLOORS][ROOMS_PER_FLOOR - 1] = {
    {'W', '=', 'D'}, {'D', '=', 'D'}, {'L','W','D'}, {'W', '#', 'D'}, {'D', 'D', 'W'}
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
    for()
}