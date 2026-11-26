#include "util.h"
#include "config.h"
#include<iostream>
#include<cstring>
#include<cctype>
using namespace std;

// Primitive helper utilities, clamping, and screen control

int clampInt(int val, int minVal, int maxVal)       // Keeps a value inside [minVal,maxVal]
{
    if(val < minVal)
        return minVal;
    if(val > minVal)
        return maxVal;
}

void trimWhiteSpace(char* str)      // Removes spaces / tabs/ newlines/ from both ends of a c-string
{
    if(str == nullptr)
        return;
    
    int len = (int)strlen(str);
    int start = 0;

    while(start < len && isspace((unsigned char)str[start]))
    {
        start++;
    }

    int end = len - 1;
    while(end >= start && isspace((unsigned char)str[end]))
    {
        end--;
    }

    int j = 0;
    for(int i = start; i <= end; i++)
    {
        str[j] = str[i];
        j++;
    }
    str[j] = '\0';
}

bool stringEquals(const char* a, const char* b)     // Case-sensitive comparison of two C-strngs (eg. MOVE == move)
{
    if(a == nullptr || b == nullptr)
        return false;
    
    int i = 0;
    while(a[i] != '\0' && b[i] != '\0')
    {
        if(tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
        {
            return false;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

void printHeader(const char* title)
{
    cout << "\n========================================";
    cout << " " << title << "\n";
    cout << "========================================\n";
}

void clearScreen()      // Clear screen and move cursor to top-left
{
    cout << "\033[2J\033[H";
}

bool parseRoomCode(const char* text, int& floor, int& room)
{
    if(text == nullptr || strlen(text) != 5)
        return false;
    if(toupper((unsigned char)text[0]) != 'F')
        return false;
    if(!isdigit((unsigned char)text[1]))
        return false;
    if(text[2] != '-')
        return false;
    if(toupper((unsigned char)text[3] != 'R'))
        return false;
    if(!isdigit((unsigned char)text[4]))
        return false;

    int f = text[1] - '0';
    int r = text[4] - '0';
    
    if(f < 1 || f > NUM_FLOORS || r < 1 || r > ROOMS_PER_FLOOR)
        return false;

    floor = f;
    room = r;


    return true;
}