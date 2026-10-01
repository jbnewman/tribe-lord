#include "events.h"

const Event EVENTS[] = {
    {
        "Strangers arrive at the village, hungry and tired.",
        { "Welcome them", .food = -10, .population = 2 },
        { "Turn them away" },
    },
    {
        "A herd of mammoths wanders past the camp.",
        { "Hunt them", .food = 15, .population = -1 },
        { "Watch and learn", .science = 3 },
    },
    {
        "A wildfire races toward the village!",
        { "Fight it", .food = -8 },
        { "Flee", .population = -1 },
    },
    {
        "A stargazer asks to stop working and study the sky.",
        { "Allow it", .food = -5, .science = 8 },
        { "Back to work", .food = 3 },
    },
    {
        "Strange seeds wash up on the riverbank.",
        { "Plant them", .food = -4, .add_tile = TILE_BERRIES },
        { "Eat them", .food = 4 },
    },
    {
        "Sickness spreads through the huts.",
        { "Rest and heal", .food = -6 },
        { "Keep working", .population = -1 },
    },
    {
        "The elders wish to paint the cave walls.",
        { "Allow it", .food = -6, .science = 5 },
        { "No time" },
    },
    {
        "A traveling trader offers a strange tool.",
        { "Trade food", .food = -12, .science = 10 },
        { "Refuse" },
    },
};

const int EVENT_COUNT = sizeof(EVENTS) / sizeof(EVENTS[0]);
