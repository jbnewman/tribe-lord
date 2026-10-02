#include <stdio.h>
#include <tonc.h>
#include "game.h"
#include "ages.h"
#include "events.h"
#include "game_config.h"
#include "game_state.h"
#include "pixel_art.h"
#include "pixel_font.h"
#include "save.h"
#include "terrain.h"
#include "tiles.h"

#define MAP_X          4
#define MAP_Y          38
#define HEX_SIZE       16
#define HEX_ROW_HEIGHT 12
#define BAR_LEFT       98
#define BAR_RIGHT      188
#define MESSAGE_Y      123
#define OFFER_Y        131
#define OFFER_WIDTH    76
#define OFFER_HEIGHT   28
#define OFFER_GAP      2
#define PANEL_LEFT     12
#define PANEL_TOP      36
#define PANEL_RIGHT    228
#define PANEL_BOTTOM   112

#define COLOR_WILD_EDGE HEX_COLOR(0x22223a)
#define COLOR_OPEN_EDGE HEX_COLOR(0x7a8aaa)
#define COLOR_CURSOR    HEX_COLOR(0xffe066)
#define COLOR_BAR       HEX_COLOR(0x4fa3d9)
#define COLOR_WINTER    HEX_COLOR(0x9be7ff)

// 'o' is the edge and '#' the inside. Rows are 12 pixels apart so the points slot together.
static const char *const HEX_SHAPE[HEX_SIZE] = {
    ".......oo.......",
    ".....oo##oo.....",
    "...oo######oo...",
    ".oo##########oo.",
    "o##############o",
    "o##############o",
    "o##############o",
    "o##############o",
    "o##############o",
    "o##############o",
    "o##############o",
    "o##############o",
    ".oo##########oo.",
    "...oo######oo...",
    ".....oo##oo.....",
    ".......oo.......",
};

static int cursor_row;
static int cursor_col;
static int selected_offer;
static const char *message;
static char message_buffer[48];
static int frame_count;
static int animation_step;
static bool management_open;
static int management_villager_index;
static bool year_summary_open;
static int year_summary_year;

#define ANIMATION_SPEED 30   // frames between animation steps (60 frames = 1 second)

static int hex_x(int row, int col)
{
    return MAP_X + col * HEX_SIZE + (row % 2) * (HEX_SIZE / 2);
}

static int hex_y(int row)
{
    return MAP_Y + row * HEX_ROW_HEIGHT;
}

static bool is_animated(int tile)
{
    return TILE_TYPES[tile].icon_alt[0] != NULL;
}

// Clears the icon's square first because icons have see-through pixels
static void draw_tile_icon(int x, int y, int tile, bool alt_frame, COLOR background)
{
    const TileType *type = &TILE_TYPES[tile];
    const char *const *icon = (alt_frame && is_animated(tile)) ? type->icon_alt : type->icon;

    m3_rect(x, y, x + TILE_ICON_SIZE, y + TILE_ICON_SIZE, background);
    draw_pixel_art(x, y, icon, TILE_ICON_SIZE, type->icon_palette, 1);
}

static void draw_terrain_icon(int x, int y, int terrain, bool alt_frame, COLOR background)
{
    const TerrainType *type = &TERRAIN_TYPES[terrain];
    const char *const *icon = (alt_frame && type->icon_alt[0] != NULL) ? type->icon_alt : type->icon;
    PaletteEntry palette[] = { { 'x', type->detail }, { 0 } };

    m3_rect(x, y, x + TERRAIN_ICON_SIZE, y + TERRAIN_ICON_SIZE, background);
    if (icon[0] != NULL) {
        draw_pixel_art(x, y, icon, TERRAIN_ICON_SIZE, palette, 1);
    }
}

// Neighboring tiles use opposite frames so the map doesn't blink all at once
static bool map_alt_frame(int row, int col)
{
    return (animation_step + row + col) % 2 == 1;
}

static bool hex_is_animated(int row, int col)
{
    int tile = game.map[row][col];
    if (tile != TILE_EMPTY) {
        return is_animated(tile);
    }
    return TERRAIN_TYPES[game.terrain[row][col]].icon_alt[0] != NULL;
}

// Mixes two colors half and half
static COLOR blend_colors(COLOR a, COLOR b)
{
    return ((a >> 1) & 0x3DEF) + ((b >> 1) & 0x3DEF);
}

// Owned land is bright, land you can build on next is medium, the rest is dark
static void hex_colors(int row, int col, COLOR *fill, COLOR *edge)
{
    const TerrainType *terrain = &TERRAIN_TYPES[game.terrain[row][col]];

    if (game.map[row][col] != TILE_EMPTY) {
        *fill = is_winter() ? terrain->winter_fill : terrain->fill;
        *edge = terrain->edge;
    } else if (is_placeable(row, col)) {
        *fill = blend_colors(terrain->dark, terrain->fill);
        *edge = COLOR_OPEN_EDGE;
    } else {
        *fill = terrain->dark;
        *edge = COLOR_WILD_EDGE;
    }

    if (row == cursor_row && col == cursor_col) {
        *edge = COLOR_CURSOR;
    }
}

static void draw_hex_icon(int row, int col, COLOR background)
{
    int x = hex_x(row, col) + 4;
    int y = hex_y(row) + 4;
    int tile = game.map[row][col];

    if (tile != TILE_EMPTY) {
        draw_tile_icon(x, y, tile, map_alt_frame(row, col), background);
    } else {
        draw_terrain_icon(x, y, game.terrain[row][col], map_alt_frame(row, col), background);
    }
}

static void draw_hex(int row, int col)
{
    COLOR fill;
    COLOR edge;
    hex_colors(row, col, &fill, &edge);

    PaletteEntry palette[] = { { 'o', edge }, { '#', fill }, { 0 } };
    draw_pixel_art(hex_x(row, col), hex_y(row), HEX_SHAPE, HEX_SIZE, palette, 1);
    draw_hex_icon(row, col, fill);
}

static void draw_map(void)
{
    m3_rect(0, MAP_Y, SCREEN_WIDTH, OFFER_Y - 1, COLOR_SKY);

    for (int row = 0; row < MAP_ROWS; row++) {
        for (int col = 0; col < MAP_COLS; col++) {
            draw_hex(row, col);
        }
    }
}

static const char *job_name(int job)
{
    switch (job) {
        case JOB_FARMER: return "Farmer";
        case JOB_SCIENTIST: return "Scientist";
        case JOB_BUILDER: return "Builder";
        case JOB_GATHERER: return "Gatherer";
        case JOB_DEFENDER: return "Defender";
        default: return "Unassigned";
    }
}

static const char *life_stage_name(int stage)
{
    switch (stage) {
        case LIFE_STAGE_INFANT: return "Infant";
        case LIFE_STAGE_CHILD: return "Child";
        case LIFE_STAGE_ADULT: return "Adult";
        default: return "Elder";
    }
}

static void draw_status(void)
{
    char line[64];

    m3_rect(0, 0, SCREEN_WIDTH, MAP_Y - 1, COLOR_SKY);

    draw_text(2, 2, "TRIBE", COLOR_TEXT, 1);

    // The turn number, or a winter warning when winter is close
    COLOR season_color = COLOR_WINTER;
    if (is_winter()) {
        snprintf(line, sizeof(line), "WINTER!");
    } else if (turns_until_winter() <= 3) {
        snprintf(line, sizeof(line), "Winter:%d", turns_until_winter());
    } else {
        snprintf(line, sizeof(line), "T%d", game.turn);
        season_color = COLOR_TEXT_DIM;
    }
    draw_text(SCREEN_WIDTH - 2 - text_width(line, 1), 2, line, season_color, 1);

    snprintf(line, sizeof(line), "F %d(%+d)  S %d(+%d)  P %d/%d",
             game.food, food_income(), game.science, science_income(), game.population, housing());
    draw_text(2, 12, line, COLOR_TEXT, 1);

    if (game.event_log_count > 0) {
        snprintf(line, sizeof(line), "%s", game.event_log[game.event_log_count - 1]);
        draw_text(2, 22, line, COLOR_TEXT_DIM, 1);
    }
    if (has_won()) {
        return;
    }
}

// Writes yields like "+2F +1S"
static void format_yields(Yields yields, char *out, int size)
{
    int length = 0;
    out[0] = '\0';

    if (yields.food > 0) {
        length += snprintf(out + length, size - length, "+%dF ", yields.food);
    }
    if (yields.science > 0) {
        length += snprintf(out + length, size - length, "+%dS ", yields.science);
    }
    if (yields.housing > 0) {
        length += snprintf(out + length, size - length, "+%dH ", yields.housing);
    }
    if (length > 0) {
        out[length - 1] = '\0';
    }
}

// Explains the hex under the cursor, or what the selected card would make there
static bool describe_cursor(char *out, int size)
{
    char yields[24];
    int tile = game.map[cursor_row][cursor_col];
    int terrain = game.terrain[cursor_row][cursor_col];

    if (tile != TILE_EMPTY) {
        format_yields(tile_yields_at(tile, cursor_row, cursor_col), yields, sizeof(yields));
        snprintf(out, size, "%s: %s", TILE_TYPES[tile].name, yields);
        return true;
    }
    if (!TERRAIN_TYPES[terrain].buildable) {
        snprintf(out, size, "%s: can't build here", TERRAIN_TYPES[terrain].name);
        return true;
    }

    int offer = game.offers[selected_offer];
    if (offer == TILE_EMPTY || !is_placeable(cursor_row, cursor_col)) {
        return false;
    }

    const char *note = "";
    if (has_terrain_bonus(offer, cursor_row, cursor_col)) {
        note = " Bonus!";
    } else if (terrain == TERRAIN_DESERT) {
        note = " (half)";
    }
    format_yields(tile_yields_at(offer, cursor_row, cursor_col), yields, sizeof(yields));
    snprintf(out, size, "%s on %s: %s%s", TILE_TYPES[offer].name, TERRAIN_TYPES[terrain].name, yields, note);
    return true;
}

static void draw_message(void)
{
    char line[48];

    m3_rect(0, MESSAGE_Y - 1, SCREEN_WIDTH, OFFER_Y - 1, COLOR_SKY);

    if (message != NULL) {
        draw_text_centered(MESSAGE_Y, message, COLOR_TEXT, 1);
    } else if (describe_cursor(line, sizeof(line))) {
        draw_text_centered(MESSAGE_Y, line, COLOR_TEXT, 1);
    } else if (can_advance_age()) {
        snprintf(line, sizeof(line), "SELECT: Enter the %s!", AGES[game.age + 1].name);
        draw_text_centered(MESSAGE_Y, line, COLOR_CURSOR, 1);
    } else {
        draw_text_centered(MESSAGE_Y, "A:Buy L/R:Card START:Turn B:Save+Quit", COLOR_TEXT_DIM, 1);
    }
}

// Writes what a tile costs, e.g. "12F 5S"
static void describe_cost(const TileType *type, char *out, int size)
{
    int length = snprintf(out, size, "%dF", type->cost_food);

    if (type->cost_science > 0) {
        snprintf(out + length, size - length, " %dS", type->cost_science);
    }
}

// Each card: icon and name, then yield, then cost (green when you can afford it) and its favorite terrain
static void draw_offers(void)
{
    char line[32];

    for (int i = 0; i < OFFER_COUNT; i++) {
        int left = 4 + i * (OFFER_WIDTH + OFFER_GAP);
        int tile = game.offers[i];
        bool is_selected = i == selected_offer;

        m3_rect(left, OFFER_Y, left + OFFER_WIDTH, OFFER_Y + OFFER_HEIGHT, COLOR_BUTTON_FILL);
        m3_frame(left, OFFER_Y, left + OFFER_WIDTH, OFFER_Y + OFFER_HEIGHT,
                 is_selected ? COLOR_BUTTON_BORDER : COLOR_BUTTON_DISABLED_BORDER);

        if (tile == TILE_EMPTY) {
            draw_text(left + (OFFER_WIDTH - text_width("SOLD", 1)) / 2, OFFER_Y + 11, "SOLD", COLOR_TEXT_DIM, 1);
            continue;
        }

        const TileType *type = &TILE_TYPES[tile];
        bool affordable = game.food >= type->cost_food && game.science >= type->cost_science;

        draw_tile_icon(left + 3, OFFER_Y + 2, tile, animation_step % 2 == 1, COLOR_BUTTON_FILL);
        draw_text(left + 14, OFFER_Y + 2, type->name, COLOR_TEXT, 1);

        format_yields((Yields){ type->food, type->science, type->housing }, line, sizeof(line));
        draw_text(left + 14, OFFER_Y + 11, line, COLOR_CURSOR, 1);

        describe_cost(type, line, sizeof(line));
        draw_text(left + 3, OFFER_Y + 20, line, affordable ? COLOR_TITLE : COLOR_TEXT_DIM, 1);

        // A small swatch of the terrain this tile likes
        if (type->terrain_bonus > 0) {
            const TerrainType *terrain = &TERRAIN_TYPES[type->bonus_terrain];
            int swatch_left = left + OFFER_WIDTH - 10;
            m3_rect(swatch_left, OFFER_Y + 19, swatch_left + 7, OFFER_Y + 26, terrain->fill);
            m3_frame(swatch_left, OFFER_Y + 19, swatch_left + 7, OFFER_Y + 26, terrain->edge);
        }
    }
}

static void draw_victory(void)
{
    char line[32];

    m3_rect(20, 40, 220, 120, COLOR_BUTTON_DISABLED_FILL);
    m3_frame(20, 40, 220, 120, COLOR_BUTTON_BORDER);
    draw_text_centered(48, "SPACE AGE!", COLOR_TITLE, 2);
    draw_text_centered(70, "Your tribe launches to", COLOR_TEXT, 1);
    draw_text_centered(80, "meet you, Overlord.", COLOR_TEXT, 1);
    snprintf(line, sizeof(line), "Reached in %d turns", game.turn);
    draw_text_centered(94, line, COLOR_TEXT_DIM, 1);
    draw_text_centered(108, "B: Main menu", COLOR_TEXT_DIM, 1);
}

static void draw_management_panel(void)
{
    char line[80];
    int infants = 0;
    int children = 0;
    int adults = 0;
    int elders = 0;

    if (!management_open || game.villager_count <= 0) {
        return;
    }

    for (int i = 0; i < game.villager_count; i++) {
        const Villager *villager = &game.villagers[i];
        if (!villager->alive) {
            continue;
        }
        switch (villager->life_stage) {
            case LIFE_STAGE_INFANT: infants++; break;
            case LIFE_STAGE_CHILD: children++; break;
            case LIFE_STAGE_ADULT: adults++; break;
            default: elders++; break;
        }
    }

    m3_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_SKY);
    draw_text_centered(4, "TRIBE ROSTER", COLOR_TITLE, 1);

    snprintf(line, sizeof(line), "Pop %d | Inf %d | Kid %d | Ad %d | Eld %d",
             game.population, infants, children, adults, elders);
    draw_text(6, 16, line, COLOR_TEXT, 1);

    Villager *villager = &game.villagers[management_villager_index];
    snprintf(line, sizeof(line), "V%d | %d yrs | %s | %s",
             villager->id, villager->age, life_stage_name(villager->life_stage), job_name(villager->job));
    draw_text(6, 26, line, COLOR_CURSOR, 1);

    int output_food = 0;
    int output_science = 0;
    int output_housing = 0;
    switch (villager->job) {
        case JOB_FARMER: output_food = 2; break;
        case JOB_SCIENTIST: output_science = 2; break;
        case JOB_BUILDER: output_housing = 1; break;
        case JOB_GATHERER: output_food = 1; break;
        case JOB_DEFENDER: break;
        default: break;
    }

    if (output_food > 0 || output_science > 0 || output_housing > 0) {
        snprintf(line, sizeof(line), "yield: %+dF %+dS %+dH",
                 output_food, output_science, output_housing);
        draw_text(6, 36, line, COLOR_TEXT_DIM, 1);
    } else if (villager->job == JOB_DEFENDER) {
        draw_text(6, 36, "yield: defense", COLOR_TEXT_DIM, 1);
    } else {
        draw_text(6, 36, "yield: none", COLOR_TEXT_DIM, 1);
    }

    draw_text(6, 46, "L/R job  A set  B close", COLOR_TEXT_DIM, 1);

    m3_rect(6, 60, SCREEN_WIDTH - 6, 100, COLOR_BUTTON_DISABLED_FILL);
    draw_text(10, 64, "History:", COLOR_TEXT, 1);
    if (game.event_log_count == 0) {
        draw_text(10, 74, "No events yet.", COLOR_TEXT_DIM, 1);
    } else {
        for (int i = 0; i < 6 && i < game.event_log_count; i++) {
            int index = game.event_log_count - 1 - i;
            draw_text(10, 74 + i * 8, game.event_log[index], COLOR_TEXT_DIM, 1);
        }
    }

    snprintf(line, sizeof(line), "%d/%d villagers", management_villager_index + 1, game.villager_count);
    draw_text(6, SCREEN_HEIGHT - 10, line, COLOR_TEXT_DIM, 1);
}

static void draw_year_summary(void)
{
    char line[80];
    int top = 38;
    int left = 20;
    int right = 220;
    int bottom = 110;

    if (!year_summary_open) {
        return;
    }

    m3_rect(left, top, right, bottom, COLOR_BUTTON_DISABLED_FILL);
    m3_frame(left, top, right, bottom, COLOR_BUTTON_BORDER);

    draw_text_centered(top + 6, "TRIBE REPORT", COLOR_TITLE, 1);
    snprintf(line, sizeof(line), "Year %d | Pop %d | Food %d | Sci %d",
             year_summary_year, game.population, game.food, game.science);
    draw_text(left + 8, top + 18, line, COLOR_TEXT, 1);

    snprintf(line, sizeof(line), "Born %d | Died %d | Net %+d",
             game.yearly_births, game.yearly_deaths, game.yearly_population_change);
    draw_text(left + 8, top + 30, line, COLOR_TITLE, 1);

    if (game.yearly_population_change > 0) {
        draw_text(left + 8, top + 41, "The tribe is growing.", COLOR_TEXT_DIM, 1);
    } else if (game.yearly_population_change < 0) {
        draw_text(left + 8, top + 41, "The tribe is shrinking.", COLOR_TEXT_DIM, 1);
    } else {
        draw_text(left + 8, top + 41, "The tribe holds steady.", COLOR_TEXT_DIM, 1);
    }

    m3_rect(left + 8, top + 52, right - 8, top + 69, COLOR_BUTTON_DISABLED_FILL);
    draw_text(left + 12, top + 55, "Recent history:", COLOR_TEXT_DIM, 1);
    for (int i = 0; i < 2 && i < game.event_log_count; i++) {
        int index = game.event_log_count - 1 - i;
        draw_text(left + 12, top + 63 + i * 7, game.event_log[index], COLOR_TEXT_DIM, 1);
    }

    draw_text_centered(bottom - 10, "A/B: Continue", COLOR_CURSOR, 1);
}

static void cycle_villager_job(int delta)
{
    if (game.villager_count <= 0) {
        return;
    }

    int next = game.villagers[management_villager_index].job + delta;
    if (next < JOB_NONE) {
        next = JOB_DEFENDER;
    }
    if (next > JOB_DEFENDER) {
        next = JOB_NONE;
    }
    game.villagers[management_villager_index].job = next;
}

static void open_management_panel(void)
{
    if (game.pending_event != NO_EVENT || game.villager_count <= 0) {
        return;
    }
    management_open = true;
    management_villager_index = 0;
    m3_fill(COLOR_SKY);
    draw_management_panel();
    draw_message();
}

// Writes an option like "A: Welcome them (-10F +2P)"
static void describe_choice(char key, const EventChoice *choice, char *out, int size)
{
    char effects[32];
    int length = 0;
    effects[0] = '\0';

    if (choice->food != 0) {
        length += snprintf(effects + length, sizeof(effects) - length, "%+dF ", choice->food);
    }
    if (choice->science != 0) {
        length += snprintf(effects + length, sizeof(effects) - length, "%+dS ", choice->science);
    }
    if (choice->population != 0) {
        length += snprintf(effects + length, sizeof(effects) - length, "%+dP ", choice->population);
    }
    if (choice->add_tile != TILE_EMPTY) {
        length += snprintf(effects + length, sizeof(effects) - length, "+card ");
    }

    if (length > 0) {
        effects[length - 1] = '\0';
        snprintf(out, size, "%c: %s (%s)", key, choice->label, effects);
    } else {
        snprintf(out, size, "%c: %s", key, choice->label);
    }
}

static bool can_afford_choice(const EventChoice *choice)
{
    return game.food + choice->food >= 0 && game.science + choice->science >= 0;
}

static void draw_event(void)
{
    const Event *event = &EVENTS[game.pending_event];
    char line[48];

    m3_rect(PANEL_LEFT, PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, COLOR_BUTTON_DISABLED_FILL);
    m3_frame(PANEL_LEFT, PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, COLOR_CURSOR);
    draw_text_centered(PANEL_TOP + 5, "EVENT", COLOR_CURSOR, 1);
    draw_text_wrapped(PANEL_LEFT + 6, PANEL_TOP + 17, event->text, COLOR_TEXT, PANEL_RIGHT - PANEL_LEFT - 12);

    describe_choice('A', &event->choice_a, line, sizeof(line));
    draw_text(PANEL_LEFT + 6, PANEL_BOTTOM - 24, line,
              can_afford_choice(&event->choice_a) ? COLOR_TITLE : COLOR_TEXT_DIM, 1);

    describe_choice('B', &event->choice_b, line, sizeof(line));
    draw_text(PANEL_LEFT + 6, PANEL_BOTTOM - 13, line,
              can_afford_choice(&event->choice_b) ? COLOR_TITLE : COLOR_TEXT_DIM, 1);
}

static void choose_event_option(bool choose_a)
{
    message = resolve_event(choose_a);

    if (game.pending_event == NO_EVENT) {
        draw_map();
        draw_status();
        draw_offers();
    }
    draw_message();
}

static void move_cursor(void)
{
    int row = cursor_row;
    int col = cursor_col;

    if (key_hit(KEY_UP))    row--;
    if (key_hit(KEY_DOWN))  row++;
    if (key_hit(KEY_LEFT))  col--;
    if (key_hit(KEY_RIGHT)) col++;

    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS) {
        return;
    }

    int old_row = cursor_row;
    int old_col = cursor_col;
    cursor_row = row;
    cursor_col = col;
    draw_hex(old_row, old_col);
    draw_hex(cursor_row, cursor_col);

    message = NULL;
    draw_message();
}

static void select_offer(int index)
{
    selected_offer = index;
    draw_offers();

    message = NULL;
    draw_message();
}

static void buy_selected_offer(void)
{
    message = buy_tile(selected_offer, cursor_row, cursor_col);

    if (message == NULL) {
        draw_map();
        draw_status();
        draw_offers();
    }
    draw_message();
}

static void try_advance_age(void)
{
    if (!advance_age()) {
        snprintf(message_buffer, sizeof(message_buffer), "Need %d science to advance.", AGES[game.age].advance_cost);
        message = message_buffer;
        draw_message();
        return;
    }

    draw_status();
    if (has_won()) {
        delete_save();
        draw_victory();
        return;
    }

    snprintf(message_buffer, sizeof(message_buffer), "Welcome to the %s!", AGES[game.age].name);
    message = message_buffer;
    draw_message();
}

static void finish_turn(void)
{
    bool was_winter = is_winter();

    message = end_turn();
    save_game();

    // Snow arrives or melts
    if (was_winter != is_winter()) {
        draw_map();
    }
    draw_status();
    draw_offers();
    draw_message();

    if ((game.turn - 1) % SEASON_LENGTH == 0) {
        year_summary_year = (game.turn - 1) / SEASON_LENGTH + 1;
        year_summary_open = true;
        draw_year_summary();
    }

    if (game.pending_event != NO_EVENT) {
        draw_event();
    }
}

// Redraws only the icons that have a second frame
static void animate_tiles(void)
{
    animation_step++;

    for (int row = 0; row < MAP_ROWS; row++) {
        for (int col = 0; col < MAP_COLS; col++) {
            if (hex_is_animated(row, col)) {
                COLOR fill;
                COLOR edge;
                hex_colors(row, col, &fill, &edge);
                draw_hex_icon(row, col, fill);
            }
        }
    }

    for (int i = 0; i < OFFER_COUNT; i++) {
        int tile = game.offers[i];
        if (is_animated(tile)) {
            int left = 4 + i * (OFFER_WIDTH + OFFER_GAP);
            draw_tile_icon(left + 3, OFFER_Y + 2, tile, animation_step % 2 == 1, COLOR_BUTTON_FILL);
        }
    }
}

// The game is set up by the New Game screen (game_new) or the menu (load_game) before this runs
void game_start(void)
{
    cursor_row = VILLAGE_ROW;
    cursor_col = VILLAGE_COL;
    selected_offer = 0;
    message = NULL;

    m3_fill(COLOR_SKY);
    year_summary_open = false;
    year_summary_year = 0;
    draw_status();
    draw_map();
    draw_message();
    draw_offers();

    if (game.pending_event != NO_EVENT) {
        draw_event();
    }
}

Screen game_update(void)
{
    if (has_won()) {
        return key_hit(KEY_B) ? SCREEN_MENU : SCREEN_GAME;
    }

    if (year_summary_open) {
        if (key_hit(KEY_A) || key_hit(KEY_B)) {
            year_summary_open = false;
            draw_status();
            draw_map();
            draw_offers();
            draw_message();
        }
        draw_year_summary();
        return SCREEN_GAME;
    }

    // While an event is open, A and B pick an option and nothing else works
    if (game.pending_event != NO_EVENT) {
        if (key_hit(KEY_A)) {
            choose_event_option(true);
        } else if (key_hit(KEY_B)) {
            choose_event_option(false);
        }
        return SCREEN_GAME;
    }

    if (management_open) {
        bool changed = false;

        if (key_hit(KEY_UP)) {
            management_villager_index = (management_villager_index + game.villager_count - 1) % game.villager_count;
            changed = true;
        }
        if (key_hit(KEY_DOWN)) {
            management_villager_index = (management_villager_index + 1) % game.villager_count;
            changed = true;
        }
        if (key_hit(KEY_L)) {
            cycle_villager_job(-1);
            changed = true;
        }
        if (key_hit(KEY_R)) {
            cycle_villager_job(1);
            changed = true;
        }
        if (key_hit(KEY_A)) {
            cycle_villager_job(1);
            changed = true;
        }
        if (key_hit(KEY_B)) {
            management_open = false;
            draw_status();
            draw_map();
            draw_offers();
            draw_message();
            return SCREEN_GAME;
        }

        if (changed) {
            m3_fill(COLOR_SKY);
            draw_management_panel();
            draw_message();
        }
        return SCREEN_GAME;
    }

    frame_count++;
    if (frame_count % ANIMATION_SPEED == 0) {
        animate_tiles();
    }

    if (key_hit(KEY_SELECT) && !can_advance_age()) {
        open_management_panel();
        if (management_open) {
            m3_fill(COLOR_SKY);
            draw_management_panel();
            draw_message();
        }
        return SCREEN_GAME;
    }

    if (key_hit(KEY_DIR)) {
        move_cursor();
    }
    if (key_hit(KEY_L)) {
        select_offer((selected_offer + OFFER_COUNT - 1) % OFFER_COUNT);
    }
    if (key_hit(KEY_R)) {
        select_offer((selected_offer + 1) % OFFER_COUNT);
    }
    if (key_hit(KEY_A)) {
        buy_selected_offer();
    }
    if (key_hit(KEY_SELECT)) {
        try_advance_age();
    }
    if (key_hit(KEY_START)) {
        finish_turn();
    }
    if (key_hit(KEY_B)) {
        save_game();
        return SCREEN_MENU;
    }
    return SCREEN_GAME;
}
