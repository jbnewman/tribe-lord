#include "ages.h"

// Reaching the last age wins the game.
const Age AGES[] = {
    { "Prehistoric Age", 15 },
    { "Stone Age",       40 },
    { "Bronze Age",      90 },
    { "Iron Age",        160 },
    { "Medieval Age",    260 },
    { "Industrial Age",  400 },
    { "Atomic Age",      600 },
    { "Space Age",       0 },
};

const int AGE_COUNT = sizeof(AGES) / sizeof(AGES[0]);
