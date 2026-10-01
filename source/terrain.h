// The land under each hex. Tiles get bonuses on certain terrain.
#ifndef TERRAIN_H
#define TERRAIN_H

#include <tonc.h>

#define TERRAIN_ICON_SIZE 8

typedef enum {
    TERRAIN_PLAINS,
    TERRAIN_FOREST,
    TERRAIN_RIVER,
    TERRAIN_HILLS,
    TERRAIN_DESERT,
    TERRAIN_TYPE_COUNT
} TerrainId;

typedef struct {
    const char *name;
    bool buildable;
    COLOR fill;             // land you own
    COLOR edge;
    COLOR winter_fill;      // land you own, during winter
    COLOR dark;             // land you don't own yet
    COLOR detail;           // color of the icon drawn on land you don't own
    const char *icon[TERRAIN_ICON_SIZE];        // optional, 'x' pixels use the detail color
    const char *icon_alt[TERRAIN_ICON_SIZE];    // optional second animation frame
} TerrainType;

extern const TerrainType TERRAIN_TYPES[TERRAIN_TYPE_COUNT];

#endif
