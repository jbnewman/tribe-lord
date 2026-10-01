// Everything about the game in progress, plus the rules that change it.
// Screens read this and call these functions; they never change the numbers directly.
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <tonc.h>
#include "terrain.h"
#include "tiles.h"

#define MAP_ROWS    8
#define MAP_COLS    14
#define VILLAGE_ROW 3
#define VILLAGE_COL 6
#define DECK_MAX    96
#define OFFER_COUNT 3

// Every SEASON_LENGTH turns, the last WINTER_LENGTH are winter
#define SEASON_LENGTH 8
#define WINTER_LENGTH 2

typedef struct {
    int food;
    int science;
    int housing;
} Yields;

typedef struct {
    int turn;
    int age;                    // index into AGES[]
    int food;
    int science;
    int population;
    int growth;                 // progress toward the next worker
    int food_bonus;             // species trait totals, in percent
    int science_bonus;
    int growth_bonus;
    u8 map[MAP_ROWS][MAP_COLS]; // a TileId for every hex
    u8 terrain[MAP_ROWS][MAP_COLS]; // a TerrainId for every hex
    u8 deck[DECK_MAX];          // tiles that can show up as offers
    int deck_size;
    u8 offers[OFFER_COUNT];     // TILE_EMPTY once bought
    int pending_event;          // index into EVENTS[] waiting for a choice, or NO_EVENT
} GameState;

extern GameState game;

// Index into SPECIES[] picked on the New Game screen
extern int chosen_species;

void game_new(void);

// These return a message to show the player, or NULL
const char *end_turn(void);
const char *buy_tile(int offer, int row, int col);
const char *resolve_event(bool choose_a);

bool can_advance_age(void);
bool advance_age(void);
bool has_won(void);
bool is_placeable(int row, int col);
bool is_winter(void);
int turns_until_winter(void);

// What a tile would produce on a given hex, counting terrain bonuses
Yields tile_yields_at(TileId tile, int row, int col);
bool has_terrain_bonus(TileId tile, int row, int col);

int food_upkeep(void);
int food_income(void);
int science_income(void);
int housing(void);
int growth_needed(void);

#endif
