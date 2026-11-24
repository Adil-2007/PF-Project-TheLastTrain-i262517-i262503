#include "physics.h"
#include "switches.h"
#include "util.h"

// Movement validation and resource adjustments without structures

static const int STAIRS[3][4] = 
{
    {1,1,2,1},  //S1: F1R1 || F2-R1
    {2,1,3,1},
    {4,1,5,1}
};


// Lift 1 stop
static bool isLift1Room(int floor, int room)
{
    return (floor == 1 || floor == 2) && (room == 2 || room == 3);
}

// Lift 2 stop

static bool isLiftRoom(int floor, int room) 
{
    return room == 4 && (floor == 2 || floor == 4 || floor == 5);
}




// Returns true if there is a passage and false for 
// solid wall

bool isDirectlyConnected(int fromF, int fromR ,int toF, int toR,
                        const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                         char& outConnType)

{
    outConnType = CONN_NONE;
    if (fromF == toF && fromR == toR) return false;

    //Neighbouring rooms on the same floor
    if (fromF == toF && (fromR - toR == 1 || toR - fromR == 1)) {
        int gap = (fromR < toR ? fromR : toR) - 1;
        outConnType = horizontalConn[fromF - 1][gap];
        if (outConnType == CONN_DOOR || outConnType == CONN_WINDOW || outConnType == CONN_LOCKED_DOOR) {
            return true;
        }
        if (outConnType == CONN_SOLID_WALL) return false;
        // no door here -> maybe a lift (not on the same floor) -> keep checking below
        outConnType = CONN_NONE;
    }

    //Staircases (both directions)
    for (int s = 0; s < 3; s++) {
        bool forward  = (fromF == STAIRS[s][0] && fromR == STAIRS[s][1] && toF == STAIRS[s][2] && toR == STAIRS[s][3]);
        bool backward = (fromF == STAIRS[s][2] && fromR == STAIRS[s][3] && toF == STAIRS[s][0] && toR == STAIRS[s][1]);
        if (forward || backward) 
        {
            outConnType = CONN_STAIRS;
            return true;
        }
    }

    //Lift 1 (between floor 1 and floor 2)
    if (isLift1Room(fromF, fromR) && isLift1Room(toF, toR) && canUseLift1(fromF, toF)) {
        outConnType = CONN_LIFT1;
        return true;
    }

    //Lift 2 (floors 2, 4, 5 - room 4)
    if (isLift2Room(fromF, fromR) && isLift2Room(toF, toR) && fromF != toF) 
    {
        outConnType = CONN_LIFT2;
        return true;
    }

    return false;   
}

// Checks the player is allowed to used the passage that exists

bool canTraverse(int targetFloor, int targetRoom, char connType,
    const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount,
                 int emergencyTripsLeft, bool lift2Operational) {
    (void)targetFloor;
    (void)targetRoom;

    if (connType == CONN_DOOR || connType == CONN_WINDOW ||
        connType == CONN_STAIRS || connType == CONN_LIFT1) {
        return true;
    }
    if (connType == CONN_LOCKED_DOOR) {
        return unlockFloor3Door(inventory, inventoryCount);
    }
    if (connType == CONN_LIFT2) {
        if (lift2Operational) return true;
        return hasItem(inventory, inventoryCount, ENT_EMERGENCY_CARD) && emergencyTripsLeft > 0;
    }
    return false; // wall 
}

void applyResourceDelta(int& energy, int& time, int deltaEnergy, int deltaTime) 
{
    energy = clampInt(energy + deltaEnergy, 0, MAX_ENERGY);
    time = clampInt(time + deltaTime, 0, MAX_TIME);
}

void applyScoreDelta(int& score, int deltaScore) 
{
    score = score + deltaScore;
    if (score < 0) score = 0;
}

bool hasItem(const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount, char itemSymbol) 
{
    for (int i = 0; i < inventoryCount; i++) {
        if (inventory[i] == itemSymbol) return true;
    }
    return false;
}

bool addItem(char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount, char itemSymbol) 
{
    if (inventoryCount >= MAX_INVENTORY_CAPACITY) return false;
    inventory[inventoryCount] = itemSymbol;
    inventoryCount++;
    return true;
}

bool removeItem(char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount, char itemSymbol) 
{
    for (int i = 0; i < inventoryCount; i++) {
        if (inventory[i] == itemSymbol) {
            // shift everything after it one place to the left
            for (int j = i; j < inventoryCount - 1; j++) {
                inventory[j] = inventory[j + 1];
            }
            inventoryCount--;
            inventory[inventoryCount] = ENT_NONE;
            return true;
        }
    }
    return false;
}