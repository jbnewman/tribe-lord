// Every playable species. Add a new entry in species_data.c and it appears on the New Game screen.
#ifndef SPECIES_DATA_H
#define SPECIES_DATA_H

#include "pixel_art.h"

#define MAX_TRAITS            4
#define MAX_PALETTE           8
#define SPECIES_SPRITE_WIDTH  8
#define SPECIES_SPRITE_HEIGHT 12

// Bonuses are whole-number percents (the GBA has no fast decimals): 20 means +20%, -10 means -10%.
typedef struct {
    const char *name;
    const char *description;
    int food_bonus;
    int science_bonus;
    int population_bonus;
} Trait;

// Unused trait and palette slots are left empty, which marks the end of each list.
typedef struct {
    const char *name;
    const char *description;
    int start_population;
    int start_food;
    int start_science;
    Trait traits[MAX_TRAITS];
    PaletteEntry palette[MAX_PALETTE];
    const char *sprite[SPECIES_SPRITE_HEIGHT];
} Species;

extern const Species SPECIES[];
extern const int SPECIES_COUNT;

#endif
