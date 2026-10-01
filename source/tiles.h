// Every kind of hex tile. Add a new tile here, give it an unlock age, and it joins the deck in that age.
#ifndef TILES_H
#define TILES_H

#include "pixel_art.h"
#include "terrain.h"

#define TILE_ICON_SIZE 8
#define MAX_ICON_COLORS 5
#define NOT_IN_DECK -1

typedef enum {
    TILE_EMPTY,
    TILE_VILLAGE,
    TILE_BERRIES,
    TILE_FIRE_PIT,
    TILE_HUT,
    TILE_HUNT_CAMP,
    TILE_CAVE_ART,
    TILE_FARM,
    TILE_TEMPLE,
    TILE_HOUSE,
    TILE_GRANARY,
    TILE_ACADEMY,
    TILE_TOWN,
    TILE_MILL,
    TILE_LIBRARY,
    TILE_KEEP,
    TILE_RANCH,
    TILE_UNIVERSITY,
    TILE_APARTMENTS,
    TILE_HYDROPONIC,
    TILE_LAB,
    TILE_TOWER,
    TILE_TYPE_COUNT
} TileId;

typedef struct {
    const char *name;           // 10 letters max so it fits on an offer card
    int food;                   // gained every turn
    int science;                // gained every turn
    int housing;                // room for this many more workers
    int cost_food;
    int cost_science;
    int unlock_age;             // index into AGES[], or NOT_IN_DECK
    TerrainId bonus_terrain;    // built on this terrain (or next to a River) adds terrain_bonus
    int terrain_bonus;          // added to the tile's main yield
    PaletteEntry icon_palette[MAX_ICON_COLORS];
    const char *icon[TILE_ICON_SIZE];
    const char *icon_alt[TILE_ICON_SIZE];   // optional second frame; leave empty for no animation
} TileType;

extern const TileType TILE_TYPES[TILE_TYPE_COUNT];

#endif
