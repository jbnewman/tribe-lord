// Random events that ask the player to choose between two options.
#ifndef EVENTS_H
#define EVENTS_H

#include "tiles.h"

#define EVENT_CHANCE 30     // percent chance each turn
#define NO_EVENT     -1

typedef struct {
    const char *label;      // short, e.g. "Welcome them"
    int food;
    int science;
    int population;
    TileId add_tile;        // added to the deck, or TILE_EMPTY for none
} EventChoice;

typedef struct {
    const char *text;       // about 100 letters max so it fits in 3 lines
    EventChoice choice_a;
    EventChoice choice_b;
} Event;

extern const Event EVENTS[];
extern const int EVENT_COUNT;

#endif
