#include <string.h>
#include <stdio.h>
#include "game_state.h"
#include "ages.h"
#include "events.h"
#include "random.h"
#include "species_data.h"

#define COPIES_PER_UNLOCK 3

GameState game;
int chosen_species = 0;
static char yearly_message[96];

static int villager_age_stage_for_age(int age, const Species *species)
{
    if (age < species->childhood_age) {
        return LIFE_STAGE_INFANT;
    }
    if (age < species->adult_age) {
        return LIFE_STAGE_CHILD;
    }
    if (age < species->elder_age) {
        return LIFE_STAGE_ADULT;
    }
    return LIFE_STAGE_ELDER;
}

int villager_life_stage_for_age(int age, const Species *species)
{
    return villager_age_stage_for_age(age, species);
}

static void set_villager_defaults(Villager *villager, int id, int age, int gender)
{
    const Species *species = &SPECIES[chosen_species];
    villager->id = id;
    villager->age = age;
    villager->gender = gender;
    villager->job = JOB_NONE;
    villager->alive = true;
    villager->life_stage = villager_age_stage_for_age(age, species);
    villager->fertility = species->fertility_cap / 2 + random_range(0, species->fertility_cap / 2);
    villager->hardiness = 45 + random_range(0, 55);
}

void log_event(const char *text)
{
    if (game.event_log_count >= MAX_EVENT_LOG) {
        memmove(game.event_log[0], game.event_log[1], (MAX_EVENT_LOG - 1) * sizeof(game.event_log[0]));
        game.event_log_count = MAX_EVENT_LOG - 1;
    }

    snprintf(game.event_log[game.event_log_count], sizeof(game.event_log[0]), "%s", text);
    game.event_log_count++;
}

void game_init_villagers(void)
{
    const Species *species = &SPECIES[chosen_species];
    game.villager_count = 0;
    game.event_log_count = 0;
    game.yearly_births = 0;
    game.yearly_deaths = 0;
    game.yearly_population_change = 0;

    for (int i = 0; i < game.population && i < MAX_VILLAGERS; i++) {
        Villager *villager = &game.villagers[i];
        int age = species->adult_age - 5 + random_range(0, 10);
        set_villager_defaults(villager, i + 1, age, random_range(0, 3));
        game.villager_count++;
    }
}

void age_villagers(int years)
{
    const Species *species = &SPECIES[chosen_species];

    for (int i = 0; i < game.villager_count; i++) {
        Villager *villager = &game.villagers[i];
        if (!villager->alive) {
            continue;
        }

        villager->age += years;
        villager->life_stage = villager_age_stage_for_age(villager->age, species);

        if (villager->life_stage == LIFE_STAGE_ADULT) {
            villager->fertility = species->fertility_cap / 2 + random_range(0, species->fertility_cap / 2);
        } else if (villager->life_stage == LIFE_STAGE_ELDER) {
            villager->fertility /= 2;
        }
    }
}

static int villager_fertility_chance(const Villager *villager, const Species *species)
{
    if (villager->gender != VILLAGER_GENDER_FEMALE || !villager->alive) {
        return 0;
    }

    if (villager->age < species->fertility_start_age || villager->age > species->fertility_end_age) {
        return 0;
    }

    int peak = species->fertility_peak_age;
    int distance = villager->age > peak ? villager->age - peak : peak - villager->age;
    int chance = 12 - distance;
    if (chance < 1) {
        chance = 1;
    }
    if (chance > 18) {
        chance = 18;
    }

    chance = chance * (villager->fertility + 10) / 28;
    if (chance < 1) {
        chance = 1;
    }
    if (chance > 18) {
        chance = 18;
    }
    return chance;
}

static void refresh_villager_count(void)
{
    int alive_count = 0;
    for (int i = 0; i < game.villager_count; i++) {
        if (game.villagers[i].alive) {
            alive_count++;
        }
    }
    game.population = alive_count;
    game.villager_count = alive_count;
}

static int job_count(int target_job)
{
    int count = 0;
    for (int i = 0; i < game.villager_count; i++) {
        if (game.villagers[i].alive && game.villagers[i].job == target_job) {
            count++;
        }
    }
    return count;
}

static int job_food_bonus(void)
{
    return job_count(JOB_FARMER) * 2 + job_count(JOB_GATHERER);
}

static int job_science_bonus(void)
{
    return job_count(JOB_SCIENTIST) * 2 + job_count(JOB_BUILDER);
}

static int job_housing_bonus(void)
{
    return job_count(JOB_BUILDER);
}

static void add_new_villager(void)
{
    const Species *species = &SPECIES[chosen_species];

    if (game.villager_count >= MAX_VILLAGERS) {
        return;
    }

    Villager *villager = &game.villagers[game.villager_count];
    set_villager_defaults(villager, game.villager_count + 1, 0, random_range(0, 3));
    villager->life_stage = LIFE_STAGE_INFANT;
    villager->age = 0;
    villager->fertility = species->fertility_cap / 2 + random_range(0, species->fertility_cap / 2);
    game.villager_count++;
    game.population++;
}

static void process_yearly_births(void)
{
    const Species *species = &SPECIES[chosen_species];
    int births = 0;
    game.yearly_births = 0;

    for (int i = 0; i < game.villager_count; i++) {
        Villager *villager = &game.villagers[i];
        if (!villager->alive) {
            continue;
        }

        int chance = villager_fertility_chance(villager, species);
        if (chance > 0 && random_range(0, 100) < chance) {
            add_new_villager();
            births++;
        }
    }

    game.yearly_births = births;
    if (births > 0) {
        snprintf(yearly_message, sizeof(yearly_message), "%d newborns join the tribe!", births);
        log_event(yearly_message);
    }
}

static int villager_mortality_chance(const Villager *villager, const Species *species)
{
    if (!villager->alive) {
        return 0;
    }

    if (villager->age < species->adult_age) {
        return 0;
    }

    int chance = 1;
    if (villager->age >= species->elder_age) {
        chance = (villager->age - species->elder_age) * 3 + 4;
    }

    if (villager->age >= species->avg_lifespan) {
        chance += 8;
    }

    chance -= villager->hardiness / 18;
    if (chance < 0) {
        chance = 0;
    }
    if (job_count(JOB_DEFENDER) > 0 && villager->job == JOB_DEFENDER) {
        chance -= 2;
        if (chance < 0) {
            chance = 0;
        }
    }
    if (chance > 100) {
        chance = 100;
    }
    return chance;
}

static void process_yearly_deaths(void)
{
    const Species *species = &SPECIES[chosen_species];
    int deaths = 0;
    game.yearly_deaths = 0;

    for (int i = 0; i < game.villager_count; i++) {
        Villager *villager = &game.villagers[i];
        if (!villager->alive) {
            continue;
        }

        int chance = villager_mortality_chance(villager, species);
        if (chance > 0 && random_range(0, 100) < chance) {
            villager->alive = false;
            deaths++;
        }
    }

    game.yearly_deaths = deaths;
    if (deaths > 0) {
        snprintf(yearly_message, sizeof(yearly_message), "%d villagers died this year!", deaths);
        log_event(yearly_message);
        refresh_villager_count();
    }
}

static void process_yearly_village_update(void)
{
    const Species *species = &SPECIES[chosen_species];
    int age_events = 0;
    game.yearly_births = 0;
    game.yearly_deaths = 0;
    game.yearly_population_change = 0;

    for (int i = 0; i < game.villager_count; i++) {
        Villager *villager = &game.villagers[i];
        if (!villager->alive) {
            continue;
        }

        int old_stage = villager->life_stage;
        villager->age += 1;
        villager->life_stage = villager_age_stage_for_age(villager->age, species);
        if (old_stage != villager->life_stage) {
            age_events++;
        }
    }

    process_yearly_births();
    process_yearly_deaths();
    game.yearly_population_change = game.yearly_births - game.yearly_deaths;

    if (game.event_log_count > 0) {
        snprintf(yearly_message, sizeof(yearly_message), "%s", game.event_log[game.event_log_count - 1]);
    } else if (age_events > 0) {
        snprintf(yearly_message, sizeof(yearly_message), "%d villagers reached a new life stage.", age_events);
        log_event(yearly_message);
    } else {
        snprintf(yearly_message, sizeof(yearly_message), "The tribe ages another year.");
        log_event(yearly_message);
    }
}

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
    int grown = with_bonus(map_yields().food, game.food_bonus) + job_food_bonus();
    if (is_winter()) {
        grown /= 2;
    }
    return grown - food_upkeep();
}

// Workers turn raw science into output, with a minor boost from job assignments.
int science_income(void)
{
    return with_bonus(map_yields().science + game.population / 2, game.science_bonus) + job_science_bonus();
}

int housing(void)
{
    return map_yields().housing + job_housing_bonus();
}

int growth_needed(void)
{
    return 6 + game.population * 2;
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
    game.villager_count = 0;
    generate_terrain();
    game_init_villagers();
    log_event("The tribe settles into the valley.");
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
            refresh_villager_count();
            message = "Starvation! A worker was lost.";
        }
    } else if (food_change > 0 && game.population < housing()) {
        // Workers only grow from spare food, and only while there is room for them
        game.growth += with_bonus(food_change, game.growth_bonus);
        if (game.growth >= growth_needed()) {
            game.growth -= growth_needed();
            game.population++;
            add_new_villager();
            message = "A new worker joins the tribe!";
        }
    }

    // The screen shows the event and asks the player to choose
    if (random_range(0, 100) < EVENT_CHANCE) {
        game.pending_event = random_range(0, EVENT_COUNT);
    }

    game.turn++;
    if ((game.turn - 1) % SEASON_LENGTH == 0) {
        process_yearly_village_update();
        if (message == NULL) {
            message = yearly_message;
        }
    }
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
