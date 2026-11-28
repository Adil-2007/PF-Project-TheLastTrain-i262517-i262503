#include "simulator.h"
#include "physics.h"
#include "switches.h"
#include "logger.h"
#include "viewer_iface.h"
#include "util.h"
#include "level.h"
#include <iostream>
#include <cstdio>
#include <cstring>
using namespace std;


// Modular simulation loop using primitive variables
static const char* SAVE_FILE    = "out/savegame.txt";
static const char* SUMMARY_FILE = "out/game_summary.txt";

static bool isAt(int floor, int room, int wantFloor, int wantRoom) 
{
    return floor == wantFloor && room == wantRoom;
}

// Returns the position of an item in a room.
static int findInRoom(const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                      int floor, int room, int entityCount, char itemSymbol) 
{
    for (int e = 0; e < entityCount; e++) 
    {
        if (roomEntities[floor - 1][room - 1][e] == itemSymbol) 
            return e;
    }
    return -1;
}

static void printHelp() 
{
    printHeader("COMMANDS");
    cout << "  move F#-R#     Move to a connected room \n";
    cout << "  inspect        Search this room \n";
    cout << "  collect        Pick up an item, e\n";
    cout << "  use            Use an item  \n";
    cout << "  unlock         Unlock the cabinet in F5-R1\n";
    cout << "  board          Board the train at F5-R2\n";
    cout << "  status | map | objectives | help\n";
    cout << "  save | load | quit\n";
    cout << "  Moves: door/stairs , window , lift\n";
}


// Move
bool handleMoveCommand(int targetFloor, int targetRoom,
                       int& currentFloor, int& currentRoom,
                       int& energy, int& time,
                       const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                       const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount,
                       int& emergencyTripsLeft, bool lift2Operational) 
{
    
    char conn = CONN_NONE;
    if (!isDirectlyConnected(currentFloor, currentRoom, targetFloor, targetRoom, horizontalConn, conn)) 
    {
        if (conn == CONN_SOLID_WALL) 
        {
            cout << "A solid wall blocks the way.No passage here.\n";
        } 
        else 
        {
            cout << "There is no direct passage from F" << currentFloor << "-R" << currentRoom
                      << " to F" << targetFloor << "-R" << targetRoom << ".\n";
        }
        return false;
    }

    if (!canTraverse(targetFloor, targetRoom, conn, inventory, inventoryCount,
                     emergencyTripsLeft, lift2Operational)) 
    {
        if (conn == CONN_LOCKED_DOOR) 
        {
            cout << "The door is locked. You need the Master Key.\n";
        } 
        else if (conn == CONN_LIFT2) 
        {
            cout << "Lift 2 is offline. Install the Fuse and collect the Lift Pass,\n"
                      << "or carry the Emergency Access Card (emergency trips left: "
                      << emergencyTripsLeft << ").\n";
        } 
        else 
        {
            cout << "You cannot go that way.\n";
        }
        return false;
    }

    int dE = COST_MOVE_ORDINARY_ENERGY;
    int dT = COST_MOVE_ORDINARY_TIME;
    if (conn == CONN_WINDOW) 
    {
        dE = COST_MOVE_WINDOW_ENERGY;
        dT = COST_MOVE_WINDOW_TIME;
    } 
    else if (conn == CONN_LIFT1 || conn == CONN_LIFT2) 
    {
        dE = COST_MOVE_LIFT_ENERGY;
        dT = COST_MOVE_LIFT_TIME;
    }

    // Before the repair, every Lift 2 ride uses one emergency trip
    if (conn == CONN_LIFT2 && !lift2Operational) 
    {
        emergencyTripsLeft--;
        std::cout << "(Emergency Access Card used - trips left: " << emergencyTripsLeft << ")\n";
    }

    applyResourceDelta(energy, time, dE, dT);
    currentFloor = targetFloor;
    currentRoom = targetRoom;

    if (conn == CONN_DOOR)   
        cout << "You walk through the door";
    else if (conn == CONN_LOCKED_DOOR) 
        cout << "You unlock the door with the Master Key and walk through";
    else if (conn == CONN_WINDOW)      
        cout << "You climb through the window";
    else if (conn == CONN_STAIRS)      
        cout << "You take the stairs";
    else if (conn == CONN_LIFT1)       
        cout << "You ride Lift 1";
    else                               
        cout << "You ride Lift 2";

    cout << " into F" << currentFloor << "-R" << currentRoom
    << " (" << getRoomName(currentFloor, currentRoom) << ").\n";
    return true;
}


// Inspect
bool handleInspectCommand(int currentFloor, int currentRoom,
                          int& energy, int& time,
                          const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                          int entityCount,
                          bool objectives[NUM_OBJECTIVES], int& score) 
{

    applyResourceDelta(energy, time, COST_INSPECT_ENERGY, COST_INSPECT_TIME);
    renderRoomDetails(currentFloor, currentRoom, roomEntities, entityCount);



    const char clueSymbols[3] = { ENT_CLUE_4, ENT_CLUE_2, ENT_CLUE_7 };
    const int clueObjs[3] = { OBJ_CLUE_4, OBJ_CLUE_2, OBJ_CLUE_7 };

    for (int i = 0; i < 3; i++) {
        if (findInRoom(roomEntities, currentFloor, currentRoom, entityCount, clueSymbols[i]) != -1 &&
            !objectives[clueObjs[i]]) 
        {
            objectives[clueObjs[i]] = true;
            applyScoreDelta(score, SCORE_CLUE_FOUND);
            std::cout << "  >> You found clue digit " << clueSymbols[i] << "! (+"
                      << SCORE_CLUE_FOUND << " points)\n";
        }
    }
    return true;
}


// Collect
bool handleCollectCommand(char itemSymbol, int currentFloor, int currentRoom,
                          int& energy, int& time, int& score,
                          char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                          int&  entityCount,
                          char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount,
                          bool objectives[NUM_OBJECTIVES]) 
{
    
    int pos = findInRoom(roomEntities, currentFloor, currentRoom, entityCount, itemSymbol);
    if (pos == -1) 
    {
        std::cout << "There is no '" << itemSymbol << "' in this room.\n";
        return false;
    }

    // Things that cannot be picked up
    if (itemSymbol == ENT_CONTROL_PANEL) 
    {
        cout << "The Control Panel is fixed to the wall. Try: use *\n";
        return false;
    }
    if (itemSymbol == ENT_PASSENGER) 
    {
        cout << "The passenger is injured. Treat them with the Medical Kit: use %\n";
        return false;
    }
    if (itemSymbol == ENT_CAT) 
    {
        cout << "The cat runs away! You need the Cat Carrier: use @\n";
        return false;
    }
    if (itemSymbol == ENT_CLUE_4 || itemSymbol == ENT_CLUE_2 || itemSymbol == ENT_CLUE_7) 
    {
        cout << "Clue digits are found by inspecting the room: inspect\n";
        return false;
    }
    if (itemSymbol == ENT_CABINET_TRAIN_KEY && !objectives[OBJ_CABINET_UNLOCKED]) 
    {
        cout << "The cabinet is locked. Find the three clue digits, then: unlock <code>\n";
        return false;
    }

    bool consumable = (itemSymbol == ENT_ENERGY_PACK || itemSymbol == ENT_EMERGENCY_TOOLKIT ||
                       itemSymbol == ENT_OBSERVATION_TOKEN);
    if (!consumable) 
    {
        if (!addItem(inventory, inventoryCount, itemSymbol)) 
        {
            std::cout << "Your inventory is full.\n";
            return false;
        }
    }

    // Remove the item from the room
    for (int e = pos; e < entityCount - 1; e++) 
    {
        roomEntities[currentFloor - 1][currentRoom - 1][e] = roomEntities[currentFloor - 1][currentRoom - 1][e + 1];
    }

    entityCount--;
    roomEntities[currentFloor - 1][currentRoom - 1][entityCount] = ENT_NONE;

    applyResourceDelta(energy, time, COST_ITEM_PICKUP_ENERGY, COST_ITEM_PICKUP_TIME);
    cout << "You picked up: [" << itemSymbol << "] " << getEntityName(itemSymbol) << ".\n";



    // Effects of each item
    if (itemSymbol == ENT_ENERGY_PACK) 
    {
        applyResourceDelta(energy, time, BONUS_ENERGY_PACK, 0);
        cout << "  >> +" << BONUS_ENERGY_PACK << " Energy\n";
    } 
    else if (itemSymbol == ENT_EMERGENCY_TOOLKIT) 
    {
        applyResourceDelta(energy, time, BONUS_EMERGENCY_TOOLKIT, 0);
        cout << "  >> +" << BONUS_EMERGENCY_TOOLKIT << " Energy\n";
    } 
    else if (itemSymbol == ENT_OBSERVATION_TOKEN) 
    {
        applyResourceDelta(energy, time, 0, BONUS_OBSERVATION_TOKEN);
        cout << "  >> +" << BONUS_OBSERVATION_TOKEN << " Time\n";
    } 

    else if (itemSymbol == ENT_MASTER_KEY) 
    {
        objectives[OBJ_MASTER_KEY] = true;
        applyScoreDelta(score, SCORE_KEY_COLLECT);
        cout << "  >> +" << SCORE_KEY_COLLECT << " points. It opens the locked door on Floor 3.\n";
    } 

    else if (itemSymbol == ENT_FUSE) 
    {
        objectives[OBJ_FUSE_COLLECTED] = true;
        cout << "  >> Take it to the Control Panel in F3-R4.\n";
    } 
    
    else if (itemSymbol == ENT_LIFT_PASS) 
    {

        objectives[OBJ_LIFT_PASS] = true;
        applyScoreDelta(score, SCORE_LIFT_PASS);
        cout << "  >> +" << SCORE_LIFT_PASS << " points.\n";

        if (objectives[OBJ_FUSE_INSTALLED]) 
        {
            objectives[OBJ_LIFT2_ACTIVE] = true;
            cout << "  >> Lift 2 is now fully operational!\n";
        }

    } 
    else if (itemSymbol == ENT_CAT_CARRIER) 
    {
        objectives[OBJ_CAT_CARRIER] = true;
        cout << "  >> Now you can secure the cat in F3-R3.\n";
    } 

    else if (itemSymbol == ENT_CABINET_TRAIN_KEY) 
    {
        objectives[OBJ_TRAIN_KEY] = true;
        applyScoreDelta(score, SCORE_TRAIN_KEY);
        cout << "  >> +" << SCORE_TRAIN_KEY << " points. You got the Train Key!\n";
    } 

    else if (itemSymbol == ENT_EMERGENCY_CARD) 
    {
        cout << "  >> It allows up to 3 emergency trips on Lift 2.\n";
    }

    return true;
}

// Use

bool handleUseCommand(char itemSymbol, int currentFloor, int currentRoom,
                      int& energy, int& time, int& score,
                      char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount,
                      bool objectives[NUM_OBJECTIVES]) 
{

    if (!hasItem(inventory, inventoryCount, itemSymbol)) 
    {
        cout << "You do not have '" << itemSymbol << "' in your inventory.\n";
        return false;
    }

    if (itemSymbol == ENT_FUSE) 
    {
        if (!isAt(currentFloor, currentRoom, 3, 4)) 
        {
            cout << "The fuse must be installed at the Control Panel in F3-R4.\n";
            return false;
        }

        if (!installFuseControlPanel(inventory, inventoryCount, objectives, score)) 
        {
            cout << "The fuse is already installed.\n";
            return false;
        }

        applyResourceDelta(energy, time, COST_ITEM_USE_ENERGY, COST_ITEM_USE_TIME);
        cout << "Fuse installed at the Control Panel! (+" << SCORE_INSTALL_FUSE << " points)\n";


        if (objectives[OBJ_LIFT2_ACTIVE])
            cout << "  >> Lift 2 is now fully operational!\n";
        else 
            cout << "  >> Collect the Lift Pass to ride Lift 2 normally.\n";

            return true;
    }



    if (itemSymbol == ENT_MEDICAL_KIT) 
    {


        if (!isAt(currentFloor, currentRoom, 4, 1) || objectives[OBJ_PASSENGER_RESCUED]) 
        {
            cout << "There is nobody here to treat. The injured passenger is in F4-R1.\n";
            return false;
        }


        removeItem(inventory, inventoryCount, ENT_MEDICAL_KIT);
        objectives[OBJ_PASSENGER_RESCUED] = true;
        applyScoreDelta(score, SCORE_RESCUE_PASSENGER);
        applyResourceDelta(energy, time, COST_ITEM_USE_ENERGY, COST_ITEM_USE_TIME);


        cout << "You treated the injured passenger. Passenger rescued! (+"
                  << SCORE_RESCUE_PASSENGER << " points)\n";


        return true;
    }

    if (itemSymbol == ENT_CAT_CARRIER) 
    {


        if (!isAt(currentFloor, currentRoom, 3, 3) || objectives[OBJ_CAT_SECURED]) 
        {
            std::cout << "There is no cat here. The cat is in F3-R3.\n";
            return false;
        }


        // The carrier now holds the cat
        removeItem(inventory, inventoryCount, ENT_CAT_CARRIER);
        addItem(inventory, inventoryCount, ENT_CAT);
        objectives[OBJ_CAT_SECURED] = true;
        applyScoreDelta(score, SCORE_SECURE_CAT);
        applyResourceDelta(energy, time, COST_ITEM_USE_ENERGY, COST_ITEM_USE_TIME);
        cout << "The cat is safe inside the carrier. Cat secured! (+" << SCORE_SECURE_CAT << " points)\n";
        return true;
    }




    if (itemSymbol == ENT_CABINET_TRAIN_KEY) 
    {

        if (!isAt(currentFloor, currentRoom, 5, 2)) 
        {
            cout << "The train is at the Train Boarding Platform (F5-R2).\n";
            return false;
        }
        if (!objectives[OBJ_PASSENGER_RESCUED] || !objectives[OBJ_CAT_SECURED]) 
        {
            cout << "You cannot leave yet! Rescue the passenger and secure the cat first.\n";
            return false;
        }
        objectives[OBJ_TRAIN_PREPARED] = true;
        objectives[OBJ_MISSION_WON] = true;
        applyScoreDelta(score, SCORE_PREPARE_TRAIN);
        applyScoreDelta(score, SCORE_MISSION_VICTORY);
        applyResourceDelta(energy, time, COST_ITEM_USE_ENERGY, COST_ITEM_USE_TIME);
        cout << "You start the train with the Train Key and everyone boards! (+"
                  << SCORE_PREPARE_TRAIN << " points, +" << SCORE_MISSION_VICTORY << " victory bonus)\n";
        return true;
    }

    if (itemSymbol == ENT_MASTER_KEY) 
    {
        cout << "The Master Key is used automatically when you go through the locked door (F3-R1 <-> F3-R2).\n";
    } 
    else if (itemSymbol == ENT_EMERGENCY_CARD || itemSymbol == ENT_LIFT_PASS) 
    {
        cout << "This is used automatically when you ride Lift 2.\n";
    } 
    else 
    {
        cout << "You cannot use that here.\n";
    }
    return false;
}

// Cabinet unlock
bool handleUnlockCabinetCommand(const char* code, int currentFloor, int currentRoom,
                                bool objectives[NUM_OBJECTIVES], int& score,
                                char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                                int& entityCount) 
{


    if (findInRoom(roomEntities, currentFloor, currentRoom, entityCount, ENT_CABINET_TRAIN_KEY) == -1) 
    {
        cout << "There is no cabinet here. The cabinet is in F5-R1.\n";
        return false;
    }


    if (objectives[OBJ_CABINET_UNLOCKED]) 
    {
        cout << "The cabinet is already open. Try: collect ^\n";
        return false;
    }


    if (!objectives[OBJ_CLUE_4] || !objectives[OBJ_CLUE_2] || !objectives[OBJ_CLUE_7]) 
    {
        cout << "You do not know the code yet. Clues found: "
                  << (objectives[OBJ_CLUE_4] ? '4' : '_')
                  << (objectives[OBJ_CLUE_2] ? '2' : '_')
                  << (objectives[OBJ_CLUE_7] ? '7' : '_') << "\n";
        return false;
    }

    if (!verifyCabinetCode(code, objectives)) 
    {
        cout << "Wrong code. The cabinet stays locked.\n";
        return false;
    }

    objectives[OBJ_CABINET_UNLOCKED] = true;
    applyScoreDelta(score, SCORE_CABINET_UNLOCK);
    cout << "Click! The cabinet opens (+" << SCORE_CABINET_UNLOCK
              << " points). The Train Key is inside: collect ^\n";


    return true;
}


// End conditions
bool checkEndConditions(int energy, int time, const bool objectives[NUM_OBJECTIVES]) 
{
    if (energy <= 0 || time <= 0) 
    return true;
    if (objectives[OBJ_MISSION_WON] || objectives[OBJ_MISSION_FAILED])
    return true;

    return false;
}


// Main loop
void runSimulation(int& playerFloor, int& playerRoom,
                   int& energy, int& time, int& score,
                   char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount,
                   bool objectives[NUM_OBJECTIVES], int& emergencyTripsLeft,
                   char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                   char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                   int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR]) 
                   {
    char line[MAX_LINE_LEN];
    bool loaded = false;

    // Start menu 
    cout << "1) New game\n2) Load saved game (" << SAVE_FILE << ")\nChoice: ";
    if (!std::cin.getline(line, MAX_LINE_LEN)) 
    return;
    trimWhitespace(line);

    if (stringEquals(line, "2")) 
    {
        loaded = loadGameStateFromFile(SAVE_FILE, playerFloor, playerRoom, energy, time, score,
                                       inventory, inventoryCount, objectives, emergencyTripsLeft,
                                       roomEntities, entityCounts);
        if (loaded) 
        cout << "Saved game loaded.\n";
        else 
        cout << "Could not load a saved game. Starting a new game.\n";
    }

    // Choose the starting room
    if (!loaded && playerFloor == 0) 
    {
        renderAsciiBuilding(0, 0);
        while (true) {
            std::cout << "Choose your starting room: ";
            if (!std::cin.getline(line, MAX_LINE_LEN)) 
            return;
            trimWhitespace(line);
            int f = 0, r = 0;
            if (parseRoomCode(line, f, r) && validateStartingRoom(f, r, horizontalConn)) 
            {
                playerFloor = f;
                playerRoom = r;
                break;
            }
            cout << "Invalid starting room.\n";
        }
    }

        cout << "\nYou are in F" << playerFloor << "-R" << playerRoom << " ("
              << getRoomName(playerFloor, playerRoom) << "). Type 'help' for commands.\n";
    renderDashboard(playerFloor, playerRoom, energy, time, score, inventory, inventoryCount, emergencyTripsLeft);



    bool quit = false;
    while (!checkEndConditions(energy, time, objectives)) {
        cout << "\n[F" << playerFloor << "-R" << playerRoom << " " << getRoomName(playerFloor, playerRoom)
                  << " | E:" << energy << " T:" << time << " S:" << score << "] > ";



        if (!std::cin.getline(line, MAX_LINE_LEN)) {
            quit = true; 
            break;
        }
        trimWhitespace(line);
        if (line[0] == '\0') continue;

        char cmd[MAX_NAME_LEN] = "";
        char arg[MAX_NAME_LEN] = "";
        sscanf(line, "%31s %31s", cmd, arg);

        int& roomCount = entityCounts[playerFloor - 1][playerRoom - 1];
        bool ok = false;
        bool isAction = true; 

        if (stringEquals(cmd, "move") || stringEquals(cmd, "go")) 
        {
            int f = 0, r = 0;
            if (!parseRoomCode(arg, f, r)) 
            {
                cout << "Usage: move F#-R# \n";
            } else {
                ok = handleMoveCommand(f, r, playerFloor, playerRoom, energy, time, horizontalConn,
                                       inventory, inventoryCount, emergencyTripsLeft,
                                       objectives[OBJ_LIFT2_ACTIVE]);
            }
        } 
        else if (stringEquals(cmd, "inspect") || stringEquals(cmd, "search") || stringEquals(cmd, "look")) 
        {
            ok = handleInspectCommand(playerFloor, playerRoom, energy, time, roomEntities, roomCount,
                                      objectives, score);
        }
        else if (stringEquals(cmd, "collect") || stringEquals(cmd, "pick") || stringEquals(cmd, "take")) 
        {
            if (arg[0] == '\0') {
                std::cout << "Usage: collect <symbol>   (e.g. collect <)\n";
            }
            else 
            {
                ok = handleCollectCommand(arg[0], playerFloor, playerRoom, energy, time, score,
                                          roomEntities, roomCount, inventory, inventoryCount, objectives);
                // The Emergency Access Card gives 3 emergency trips on L2
                if (ok && arg[0] == ENT_EMERGENCY_CARD) emergencyTripsLeft = 3;
            }
        } 
        else if (stringEquals(cmd, "use")) 
        {
            if (arg[0] == '\0') 
            {
                cout << "Usage: use <symbol>   (e.g. use *)\n";
            } 
            else 
            {
                ok = handleUseCommand(arg[0], playerFloor, playerRoom, energy, time, score,
                                      inventory, inventoryCount, objectives);
            }
        } 
        else if (stringEquals(cmd, "board")) 
        {
            ok = handleUseCommand(ENT_CABINET_TRAIN_KEY, playerFloor, playerRoom, energy, time, score,
                                  inventory, inventoryCount, objectives);
        } 
        else if (stringEquals(cmd, "unlock")) 
        {
            ok = handleUnlockCabinetCommand(arg, playerFloor, playerRoom, objectives, score,
                                            roomEntities, roomCount);
            if (ok) applyResourceDelta(energy, time, COST_ITEM_USE_ENERGY, COST_ITEM_USE_TIME);
        } 
        else if (stringEquals(cmd, "status") || stringEquals(cmd, "inventory")) 
        {
            isAction = false;
            renderDashboard(playerFloor, playerRoom, energy, time, score, inventory, inventoryCount,
                            emergencyTripsLeft);
        } 
        else if (stringEquals(cmd, "map")) 
        {
            isAction = false;
            renderAsciiBuilding(playerFloor, playerRoom);
        } 
        else if (stringEquals(cmd, "objectives")) 
        {
            isAction = false;
            renderObjectivesChecklist(objectives);
        }
        else if (stringEquals(cmd, "help")) 
        {
            isAction = false;
            printHelp();
        } 


        else if (stringEquals(cmd, "save")) 
        {
            isAction = false;
            if (saveGameStateToFile(SAVE_FILE, playerFloor, playerRoom, energy, time, score, inventory,
                                    inventoryCount, objectives, emergencyTripsLeft, roomEntities, entityCounts)) 
            {
                cout << "Game saved to " << SAVE_FILE << "\n";
            } 
            else 
            {
                cout << "Could not save (does the out/ folder exist?)\n";
            }
        } 


        else if (stringEquals(cmd, "load")) 
        {


            isAction = false;
            if (loadGameStateFromFile(SAVE_FILE, playerFloor, playerRoom, energy, time, score, inventory,
                                      inventoryCount, objectives, emergencyTripsLeft, roomEntities, entityCounts)) 
            {
                cout << "Saved game loaded.\n";
            } 
            else 
            {
                cout << "Could not load " << SAVE_FILE << "\n";
            }

        } 
        else if (stringEquals(cmd, "quit") || stringEquals(cmd, "exit")) 
        {
            quit = true;
            break;
        } 
        else 
        {
            isAction = false;
            cout << "Unknown command. Type 'help'.\n";
        }

        // Audit trail for game_summary.txt
        if (isAction) 
        {
            char entry[MAX_LINE_LEN];
            snprintf(entry, MAX_LINE_LEN, "%-14.40s -> %-8s | F%d-R%d | E=%d T=%d S=%d",
                     line, ok ? "OK" : "REJECTED", playerFloor, playerRoom, energy, time, score);
            recordAuditEntry(entry);
        }

        if (energy <= 0 || time <= 0) 
        {
            objectives[OBJ_MISSION_FAILED] = true;
        }
    }


    // Game over / quit 
    if (quit) 
    {
        saveGameStateToFile(SAVE_FILE, playerFloor, playerRoom, energy, time, score, inventory,
                            inventoryCount, objectives, emergencyTripsLeft, roomEntities, entityCounts);
        cout << "\nGame saved to " << SAVE_FILE << ". Goodbye!\n";
        return;
    }

    bool victory = objectives[OBJ_MISSION_WON];

    if (victory) 
    {
        printHeader("VICTORY! THE LAST TRAIN DEPARTS SAFELY");
    } 

    else 
    {
        printHeader("MISSION FAILED");
        if (energy <= 0) 
            cout << " You ran out of Energy.\n";
        else 
            cout << " You ran out of Time.\n";
    }


    renderDashboard(playerFloor, playerRoom, energy, time, score, inventory, inventoryCount, emergencyTripsLeft);
    renderObjectivesChecklist(objectives);
    writeGameSummaryReport(SUMMARY_FILE, score, energy, time, objectives, victory);
    cout << "\nSummary written to " << SUMMARY_FILE << "\n";
}
