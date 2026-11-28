#include "simulator.h"
#include "physics.h"
#include "switches.h"
#include "logger.h"
#include "viewer_iface.h"
#include "util.h"
using namespace std;


// Modular simulation loop using primitive variables
static const char* SAVE_FILE = "out/savegame.txt";
static const char* SUMMARY_FILE = "out/game_summary.txt";

static bool isAt(int floor, int room, int wantFloor, int wantRoom)
{
    return floor == wantFloor && room == wantRoom;
}

// Return the position of a item
static int findInRoom(const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
    int floor, int room,int entityCount, char itemSymbol)
    {
        for(int e = 0;e<entityCount;e++)
        {
            if(roomEntities[floor - 1][room - 1][e] == itemSymbol)
            {
                return e;
            }
        }
        return -1;
    }


static void printHelp()
{
    printHeader("COMMANDS");
    cout << " Move F*-R* Move to a connected Room" << endl;
    cout << " Inspect    Search this room" << endl;
    cout << " Collect    Pick up an item" << endl;
    cout << " Use        Use an item" << endl;
    cout << " Unlock     Unlock the room" << endl;
    cout << " Board      Board the train" << endl;
    cout << " Status | Map | Objective | Help" << endl;
    cout << " Save | Load | Quit" << endl;
    cout << " Moves: Door/Stairs" << endl;

}


//MOVE


bool handleMoveCommand(int targetFloor, int targetRoom,
                     int& currentFloor , int& currentRoom,
                    int& energy, int& time,
                const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount,
                int& emergencyTripsLeft, bool lift20perrational)
                {
                    char conn = CONN_NONE;
                    if(!isDirectlyConnected(currentFloor , currentRoom,targetFloor,targetRoom,horizontalConn,conn))
                    {
                        if(conn == CONN_SOLID_WALL)
                        {
                            cout << "A solid wall here, There is no passage";
                        }
                        else
                        {
                            cout << "There is no direct passage from F" << currentFloor << "-R" << currentRoom << "to F" << targetFloor << "-R" << targetRoom << ".\n";

                        }
                        return false;
                    }

                    if(!canTraverse(targetFloor,targetRoom,conn, inventory,inventoryCount,
                    emergencyTripsLeft, lift20perational))
                    {
                        if(conn == CONN_LOCKED_DOOR)
                        {
                            cout << "The door is locked. You need the Master key" << endl;

                        }
                        else if(conn == CONN_LIFT2)
                        {
                            cout << "Lift is not working. Install the fuse and collect the lift pass.\n"
                                << "or carry the Emergency Access Card: "
                                << emergencyTripsLeft << ").\n";
                        }
                        else
                        {
                            cout << "You cannot go that way.\n";
                        }
                        return false;
                        }

                    }

                


int dE = COST_MOVE_ORDINARY_ENERGY;
int dT = COST_MOVE_ORDINARY_TIME;
if( conn == CONN_WINDOW)
{
    dE = COST_MOVE_WINDOW_ENERGY;
    dT = COST_MOVE_WINDOW_TIME;
}
else if(conn == CONN_LIFT1 || conn == CONN_LIFT2)
{
    dE = COST_MOVE_LIFT_ENERGY;
    dT = COST_MOVE_LIFT_TIME;
}



    // Before the repair, L2 ride uses one E trip
    if(conn == CONN_LIFT2 && !lift2operational)
    {
    emergencyTripsLeft--;
    cout << "Emergency Card Used - Trips Left " << emergencyTripsLeft << "\n";

    }

    applyResourceDelta(energy, time , dE , dT);
    currentFloor = targetFloor;
    currentRoom = targetRoom;


    if(conn == CONN_DOOR)
        cout << "You walked through the door";
    else if(conn == CONN_LOCKED_DOOR)
        cout << "You unlock the door with the Master key and walk through";
    else if (conn == CONN_WINDOW)      
        cout << "You climb through the window";
    else if (conn == CONN_STAIRS)      
        cout << "You take the stairs";
    else if (conn == CONN_LIFT1)       
        cout << "You ride Lift 1";
    else               
        cout << "You ride Lift 2";
    cout << "into F" << currentFloor << "-R" << currentRoom
        << " (" << getRoomName(currentFloor, currentRoom) << ").\n";
    return true;

}


