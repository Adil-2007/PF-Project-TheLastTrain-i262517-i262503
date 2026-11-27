#include "logger.h"
#include "util.h"
#include <fstream>
#include <cstring>

// File persistence for savegame.txt and game_summary.txt using primitive variables

// Names used for the objective flags in savegame.txt (same order as OBJ_* in state.h)

static const char* OBJECTIVE_KEYS[NUM_OBJECTIVES] = {
    "MASTER_KEY", "CLUE_4", "CLUE_2", "CLUE_7", "CABINET_UNLOCKED", "TRAIN_KEY", "FUSE_COLLECTED", "FUSE_INSTALLED", "LIFT_PASS", "LIFT2_ACTIVE", "PASSENGER_RESCUED", "CAT_CARRIER", "CAT_SECURED", "TRAIN_PREPARED", "MISSION_WON", "MISSION_FAILED"
};

// Scoring checklist for the summary (12 items, 520 points in total)

const int NUM_SCORE_ITEMS = 12;
static const int SCORE_ITEM_OBJ[NUM_SCORE_ITEMS] = {
    OBJ_MASTER_KEY, OBJ_CLUE_4, OBJ_CLUE_2, OBJ_CLUE_7,
    OBJ_CABINET_UNLOCKED, OBJ_TRAIN_KEY, OBJ_PASSENGER_RESCUED, OBJ_CAT_SECURED, OBJ_FUSE_INSTALLED, OBJ_LIFT_PASS, OBJ_TRAIN_PREPARED, OBJ_MISSION_WON
};

static const int SCORE_ITEM_POINTS[NUM_SCORE_ITEMS] = {
    SCORE_KEY_COLLECT, SCORE_CLUE_FOUND, SCORE_CLUE_FOUND, SCORE_CLUE_FOUND, SCORE_CABINET_UNLOCK, SCORE_TRAIN_KEY, SCORE_RESCUE_PASSENGER, SCORE_SECURE_CAT, SCORE_INSTALL_FUSE, SCORE_LIFT_PASS, SCORE_PREPARE_TRAIN< SCORE_MISSION_VICTORY 
};

static const char* SCORE_ITEM_LABEL[NUM_SCORE_ITEMS] = {
    "Master Key Collected", "Clue digit 4 Found", "Clue Digit 2 Found", "Clue Digit 7 Found", "Cabinet Unlocked (Code 427)", "Train Key Acquired", "Passenger Rescued (F4-R1)",
    "Cat Secured (F3-R3)", "Fuse Installed at Panel (F3-R4)", "Lift Pass Collected (F3-R4)",
    "Train Prepared & Boarded (F5-R2)", "Full Mission Victory Bonus"
};


// Session audit trail: one line per command typed during the session

const int MAX_AUDIT_LINES = 500;
static char auditLog[MAX_AUDIT_LINES][MAX_LINE_LEN];
static int auditCount = 0;

void recordAuditEntry(const char* text)
{
    if(auditCount >= MAX_AUDIT_LINES) 
        return;
    strncpy(auditLog[auditCount], text, MAX_LINE_LEN - 1);
    auditLog[auditCount][MAX_LINE_LEN - 1] = '\0';
    auditCount++;
}

bool saveGameStateToFile(const char* filename, int playerFloor, int playerRoom, int energy, int time, int score, const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount, const bool objectives[NUM_OBJECTIVES], int emergencyTripsLeft, const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM], const int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR])
{
    std::ofstream fout(filename);
    if(!fout)
        return false;

    fout << "THE_LAST_TRAIN_SAVE v1\n";
    fout << "PLAYER_FLOOR " << playerFloor << "\n";
    fout << "PLAYER_ROOM " << playerRoom << "\n";
    fout << "ENERGY " << energy << "\n";
    fout << "TIME " << time << "\n";
    fout << "SCORE " << score << "\n";
    fout << "EMERGENCY_TRIPS_LEFT " << emergencyTripsLeft << "\n";

    fout << "INVENTORY " << inventoryCount;
    for(int i = 0; i < inventoryCount; i++)
        fout << " " << inventory[i];
    fout << "\n";

    for(int i = 0; i < NUM_OBJECTIVES; i++)
    {
        fout << "OBJ " << OBJECTIVE_KEYS[i] << " " << (objectives[i] ? 1 : 0) << "\n";
    }

    for(int f = 0; f < NUM_FLOORS; f++)
    {
        for(int r = 0; r < ROOMS_PER_FLOOR; r++)
        {
            fout << "ROOM " << (f + 1) << " " << (r + 1) << " " << entityCounts[f][r];
            for(int e = 0; e < entityCounts[f][r]; e++)
                fout << " " << roomEntities[f][r][e];
            fout << "\n";
        }
    }
    return true;
}

bool loadGameStateFromFile(const char* filename, int& playerFloor, int& playerRoom, int& energy, int& time, int&score, char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount, bool objectives[NUM_OBJECTIVES], int& emergencyTripsLeft, char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM], int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR])
{
    std::ifstream fin(filename);
    if(!fin) 
        return false;

    char label[64];
    char version[64];
    fin >> label >> version;
    if(!fin || !stringEquals(label, "TEL_LAST_TRAIN_SAVE"))
        return false;

    int pf = 0, pr = 0, en = 0, tm = 0, sc = 0, trips = 0, invCount = 0;

    fin >> label >> pf >> label >> pr >> label >> en >> label >> tm >> label >> sc >> label >> trips;

    fin >> label >> invCount;

    if(!fin || pf < 1 || pf > NUM_FLOORS || pr < 1 || pr > ROOMS_PER_FLOOR)
        return false;

    if(invCount < 0 || invCount > MAX_INVENTORY_CAPACITY)
        return false;

    char inv[MAX_INVENTORY_CAPACITY];
    for(int i = 0; i < invCount; i++)
        fin >> inv[i];

    bool obj[NUM_OBJECTIVES];
    for(int i = 0; i < NUM_OBJECTIVES; i++)
    {
        char name[64];
        int value = 0;
        fin >> label >> name >> value;
        obj[i] = (value == 1);
    }

    char ents[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM];
    int counts[NUM_FLOORS][ROOMS_PER_FLOOR];
    for(int f = 0; f < NUM_FLOORS; f++)
    {
        for(int r = 0; r < ROOMS_PER_FLOOR; r++)
        {
            int ff = 0, rr = 0, c = 0;
            fin >> label >> ff >> rr >> c;
            if(!fin || c < 0 || c > MAX_ENTITIES_PER_ROOM)
                return false;
            counts[f][r] = c;
            for(int e = 0; e < MAX_ENTITIES_PER_ROOM; e++)
                ents[f][r][e] = ENT_NONE;
            for(int e = 0; e < c; e++)
                fin >> ents[f][r][e];
        }
    }
    if(!fin)
        return false;

    playerFloor = pf;
    playerRoom = pr;
    energy = clampInt(en, 0, MAX_ENERGY);
    time = clampInt(tm, 0, MAX_TIME);
    score = sc;
    emergencyTripsLeft = trips;
    inventoryCount = invCount;

    for(int i = 0; i < invCount; i++)
        inventory[i] = inv[i];
    for(int i = 0; i < NUM_OBJECTIVES; i++)
        objectives[i] = obj[i];
    for(int f = 0; f < NUM_FLOORS; f++)
    {
        for(int r = 0; r < ROOMS_PER_FLOOR; r++)
        {
            entityCounts[f][r] = counts[f][r];
            for(int e = 0; e < MAX_ENTITIES_PER_ROOM; e++)
                roomEntities[f][r][e] = ents[f][r][e];
        }
    }
    return true;
}

void writeGaemSummaryReport(const char* filename, int score, int remainingEnergy, int remainingTime, const bool objectives[NUM_OBJECTIVES], bool victory)
{
    std::ofstream fout(filename);
    if(!fout)
        return;

    fout << "===========================================\n";
    fout << " THE LAST TRAIN - FINAL SESSION REPORT\n";
    fout << "===========================================\n";
    fout << "RESULT     : " << (victory ? "VICTORY" : "DEFEAT") << "\n";
    if(!victory)
    {
        if(remainingEnergy <= 0)
            fout << "REASON     : Energy ran out\n";
        else if (remainingTime <= 0)
            fout << "REASON     : Time ran out\n";
        else
            fout << "REASON     : Mission failed\n";
    }

    fout << "FINAL SCORE    : " << score << " / 520\n";
    fout << "REMAINING ENERGY : " << remainingEnergy << " / " << MAX_ENERGY << "\n";
    fout << "REMAINING TIME : " << remainingTime << " / " << MAX_TIME << "\n\n";

    fout << "OBJECTIVES CHECKLIST\n";
    int done = 0;
    int pointsCheck = 0;
    for(int i = 0; i < NUM_SCORE_ITEMS; i++)
    {
        bool ok = objectives[SCORE_ITEM_OBJ[i]];
        if(ok)
        {
            done++;
            pointsCheck += SCORE_ITEM_POINTS[i];
        }
    }
}