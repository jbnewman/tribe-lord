#include <string.h>
#include "game_state.h"
#include "ages.h"
#include "events.h"
#include "random.h"
#include "species_data.h"

#define COPIES_PER_UNLOCK 3

GameState game;
int chosen_species = 0;

// Hex neighbors as { row, col } steps. Odd rows sit half a hex to the right, so they differ.
static const int NEIGHBORS_EVEN_ROW[6][2] = { { 0, -1 }, { 0, 1 }, { -1, -1 }, { -1, 0 }, { 1, -1 }, { 1, 0 } };
static const int NEIGHBORS_ODD_ROW[6][2]  = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { -1, 1 }, { 1, 0 }, { 1, 1 } };

// Finds neighbor number 0-5 of a hex. Returns false if it's off the map.
static bool neighbor_of(int row, int col, int index, int *neighbor_row, int *neighbor_col)
{
    const int (*steps)[2] = (row % 2 == 0) ? NEIGHBORS_EVEN_ROW : NEIGHBORS_ODD_ROW;
    *neighbor_row = row + steps[index][0];
    *neighbor_col = col + steps[index][1];
    return *neighbor_row >= 0 && *neighbor_row < MAP_ROWS && *neighbor_col >= 0 && *neighbor_col < MAP_COLS;
}

static int with_bonus(int amount, int percent)
{
    return amount + amount * percent / 100;
}

// Rivers count when next to the tile; every other terrain counts when the tile is built on it
bool has_terrain_bonus(TileId tile, int row, int col)
{
    const TileType *type = &TILE_TYPES[tile];
    if (type->terrain_bonus == 0) {
        return false;
    }
    if (type->bonus_terrain != TERRAIN_RIVER) {
        return game.terrain[row][col] == type->bonus_terrain;
    }

    int r, c;
    for (int i = 0; i < 6; i++) {
        if (neighbor_of(row, col, i, &r, &c) && game.terrain[r][c] == TERRAIN_RIVER) {
            return true;
        }
    }
    return false;
}

// The bonus goes to the tile's main yield. Desert halves tiles that don't like it.
Yields tile_yields_at(TileId tile, int row, int col)
{
    const TileType *type = &TILE_TYPES[tile];
    Yields yields = { type->food, type->science, type->housing };

    if (has_terrain_bonus(tile, row, col)) {
        if (yields.food > 0) {
            yields.food += type->terrain_bonus;
        } else if (yields.science > 0) {
            yields.science += type->terrain_bonus;
        } else {
            yields.housing += type->terrain_bonus;
        }
    } else if (game.terrain[row][col] == TERRAIN_DESERT) {
        yields.food /= 2;
        yields.science /= 2;
        yields.housing /= 2;
    }
    return yields;
}

static Yields map_yields(void)
{
    Yields total = { 0, 0, 0 };

    for (int row = 0; row < MAP_ROWS; row++) {
        for (int col = 0; col < MAP_COLS; col++) {
            if (game.map[row][col] != TILE_EMPTY) {
                Yields here = tile_yields_at(game.map[row][col], row, col);
                total.food += here.food;
                total.science += here.science;
                total.housing += here.housing;
            }
        }
    }
    return total;
}

bool is_winter(void)
{
    return (game.turn - 1) % SEASON_LENGTH >= SEASON_LENGTH - WINTER_LENGTH;
}

// 0 or less means it is winter now
int turns_until_winter(void)
{
    return (SEASON_LENGTH - WINTER_LENGTH) - (game.turn - 1) % SEASON_LENGTH;
}

int food_upkeep(void)
{
    return game.population / 2;
}

// Food grown minus food eaten, so it can be negative. Winter halves what is grown.
int food_income(void)
{
    int grown = with_bonus(map_yields().food, game.food_bonus);
    if (is_winter()) {
        grown /= 2;
    }
    return grown - food_upkeep();
}

// Every two workers also come up with one science
int science_income(void)
{
    return with_bonus(map_yields().science + game.population / 2, game.science_bonus);
}

int housing(void)
{
    return map_yields().housing;
}

int growth_needed(void)
{
    return 5 + game.population * 2;
}

static void add_to_deck(TileId tile, int copies)
{
    for (int i = 0; i < copies && game.deck_size < DECK_MAX; i++) {
        game.deck[game.deck_size++] = tile;
    }
}

static void unlock_age_tiles(int age)
{
    for (int tile = 0; tile < TILE_TYPE_COUNT; tile++) {
        if (TILE_TYPES[tile].unlock_age == age) {
            add_to_deck(tile, COPIES_PER_UNLOCK);
        }
    }
}

static void deal_offers(void)
{
    for (int i = 0; i < OFFER_COUNT; i++) {
        game.offers[i] = game.deck[random_range(0, game.deck_size)];
    }
}

// Paints a wandering blob of one terrain
static void paint_patch(TerrainId terrain, int size)
{
    int row = random_range(0, MAP_ROWS);
    int col = random_range(0, MAP_COLS);

    for (int i = 0; i < size; i++) {
        game.terrain[row][col] = terrain;
        int r, c;
        if (neighbor_of(row, col, random_range(0, 6), &r, &c)) {
            row = r;
            col = c;
        }
    }
}

// A river flows from the top of the map to the bottom, away from the village
static void paint_river(void)
{
    int col = random_range(0, 2) == 0 ? random_range(1, 3) : random_range(10, 12);

    for (int row = 0; row < MAP_ROWS; row++) {
        game.terrain[row][col] = TERRAIN_RIVER;

        // Step to the lower-left or lower-right hex so the river stays connected
        col += (row % 2 == 0) ? random_range(-1, 1) : random_range(0, 2);
        if (col < 0) {
            col = 0;
        }
        if (col >= MAP_COLS) {
            col = MAP_COLS - 1;
        }
    }
}

static void generate_terrain(void)
{
    for (int i = 0; i < 3; i++) {
        paint_patch(TERRAIN_FOREST, 7);
    }
    for (int i = 0; i < 2; i++) {
        paint_patch(TERRAIN_HILLS, 5);
        paint_patch(TERRAIN_DESERT, 6);
    }
    paint_river();
    game.terrain[VILLAGE_ROW][VILLAGE_COL] = TERRAIN_PLAINS;
}

void game_new(void)
{
    const Species *species = &SPECIES[chosen_species];

    memset(&game, 0, sizeof(game));
    game.turn = 1;
    game.food = species->start_food;
    game.science = species->start_science;
    game.population = species->start_population;

    for (int i = 0; i < MAX_TRAITS && species->traits[i].name != NULL; i++) {
        game.food_bonus += species->traits[i].food_bonus;
        game.science_bonus += species->traits[i].science_bonus;
        game.growth_bonus += species->traits[i].population_bonus;
    }

    game.map[VILLAGE_ROW][VILLAGE_COL] = TILE_VILLAGE;
    game.pending_event = NO_EVENT;
    generate_terrain();
    unlock_age_tiles(0);
    deal_offers();
}

bool is_placeable(int row, int col)
{
    if (game.map[row][col] != TILE_EMPTY || !TERRAIN_TYPES[game.terrain[row][col]].buildable) {
        return false;
    }

    int r, c;
    for (int i = 0; i < 6; i++) {
        if (neighbor_of(row, col, i, &r, &c) && game.map[r][c] != TILE_EMPTY) {
            return true;
        }
    }
    return false;
}

const char *buy_tile(int offer, int row, int col)
{
    TileId tile = game.offers[offer];
    if (tile == TILE_EMPTY) {
        return "That card is already sold.";
    }

    const TileType *type = &TILE_TYPES[tile];
    if (game.map[row][col] != TILE_EMPTY) {
        return "That land is already used.";
    }
    if (!TERRAIN_TYPES[game.terrain[row][col]].buildable) {
        return "You can't build on a river.";
    }
    if (!is_placeable(row, col)) {
        return "Must be next to your land.";
    }
    if (game.food < type->cost_food) {
        return "Not enough food.";
    }
    if (game.science < type->cost_science) {
        return "Not enough science.";
    }

    game.food -= type->cost_food;
    game.science -= type->cost_science;
    game.map[row][col] = tile;
    game.offers[offer] = TILE_EMPTY;
    return NULL;
}

const char *resolve_event(bool choose_a)
{
    const Event *event = &EVENTS[game.pending_event];
    const EventChoice *choice = choose_a ? &event->choice_a : &event->choice_b;

    if (game.food + choice->food < 0) {
        return "Not enough food for that.";
    }
    if (game.science + choice->science < 0) {
        return "Not enough science for that.";
    }

    game.food += choice->food;
    game.science += choice->science;
    game.population += choice->population;
    if (game.population < 1) {
        game.population = 1;
    }
    if (choice->add_tile != TILE_EMPTY) {
        add_to_deck(choice->add_tile, 1);
    }

    game.pending_event = NO_EVENT;
    return NULL;
}

const char *end_turn(void)
{
    const char *message = NULL;
    int food_change = food_income();

    game.food += food_change;
    game.science += science_income();

    if (game.food < 0) {
        game.food = 0;
        game.growth = 0;
        if (game.population > 1) {
            game.population--;
            message = "Starvation! A worker was lost.";
        }
    } else if (food_change > 0 && game.population < housing()) {
        // Workers only grow from spare food, and only while there is room for them
        game.growth += with_bonus(food_change, game.growth_bonus);
        if (game.growth >= growth_needed()) {
            game.growth -= growth_needed();
            game.population++;
            message = "A new worker joins the tribe!";
        }
    }

    // The screen shows the event and asks the player to choose
    if (random_range(0, 100) < EVENT_CHANCE) {
        game.pending_event = random_range(0, EVENT_COUNT);
    }

    game.turn++;
    deal_offers();
    return message;
}

bool can_advance_age(void)
{
    return !has_won() && game.science >= AGES[game.age].advance_cost;
}

bool advance_age(void)
{
    if (!can_advance_age()) {
        return false;
    }

    game.science -= AGES[game.age].advance_cost;
    game.age++;
    unlock_age_tiles(game.age);
    return true;
}

bool has_won(void)
{
    return game.age == AGE_COUNT - 1;
}
