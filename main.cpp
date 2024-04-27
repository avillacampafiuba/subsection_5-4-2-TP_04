//=====[Libraries]=============================================================

#include "smart_home_system.h"

#define TEST_OG = 0     //ORIGINAL
#define TEST_NB = 1     //NON BLOCKING

#define TEST = (TEST_OG)

//=====[Main function, the program entry point after power on or reset]========

int main()
{
    smartHomeSystemInit();
    while (true) {
        smartHomeSystemUpdate();
    }
}