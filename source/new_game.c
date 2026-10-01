#include <stdio.h>
#include <tonc.h>
#include "new_game.h"
#include "game_config.h"
#include "game_state.h"
#include "pixel_art.h"
#include "pixel_font.h"
#include "random.h"
#include "species_data.h"

#define CARD_WIDTH   72
#define CARD_GAP     6
#define CARD_TOP     16
#define CARD_BOTTOM  62
#define SPRITE_SCALE 2

#define PANEL_LEFT   4
#define PANEL_TOP    66
#define PANEL_RIGHT  236
#define PANEL_BOTTOM 146
#define TEXT_LEFT    8
#define TEXT_WIDTH   224

static int selected;
static unsigned int frames_on_screen;

static int card_left(int index)
{
    int total_width = SPECIES_COUNT * CARD_WIDTH + (SPECIES_COUNT - 1) * CARD_GAP;
    return (SCREEN_WIDTH - total_width) / 2 + index * (CARD_WIDTH + CARD_GAP);
}

static void draw_card(int index)
{
    const Species *species = &SPECIES[index];
    int left = card_left(index);
    int center = left + CARD_WIDTH / 2;

    m3_rect(left, CARD_TOP, left + CARD_WIDTH, CARD_BOTTOM, COLOR_BUTTON_FILL);
    draw_pixel_art(center - SPECIES_SPRITE_WIDTH * SPRITE_SCALE / 2, CARD_TOP + 4,
                   species->sprite, SPECIES_SPRITE_HEIGHT, species->palette, SPRITE_SCALE);
    draw_text(center - text_width(species->name, 1) / 2, CARD_TOP + 34, species->name, COLOR_TEXT, 1);
}

// Selected cards get a 2 pixel green border, the rest a thin dim one
static void draw_card_border(int index, bool is_selected)
{
    int left = card_left(index);
    int right = left + CARD_WIDTH;

    m3_frame(left, CARD_TOP, right, CARD_BOTTOM, is_selected ? COLOR_BUTTON_BORDER : COLOR_BUTTON_DISABLED_BORDER);
    m3_frame(left + 1, CARD_TOP + 1, right - 1, CARD_BOTTOM - 1, is_selected ? COLOR_BUTTON_BORDER : COLOR_BUTTON_FILL);
}

static void draw_details(void)
{
    const Species *species = &SPECIES[selected];
    char line[64];

    m3_rect(PANEL_LEFT + 1, PANEL_TOP + 1, PANEL_RIGHT - 1, PANEL_BOTTOM - 1, COLOR_BUTTON_DISABLED_FILL);

    int y = draw_text_wrapped(TEXT_LEFT, PANEL_TOP + 4, species->description, COLOR_TEXT, TEXT_WIDTH);
    y += 2;

    for (int i = 0; i < MAX_TRAITS && species->traits[i].name != NULL; i++) {
        snprintf(line, sizeof(line), "* %s: %s", species->traits[i].name, species->traits[i].description);
        y = draw_text_wrapped(TEXT_LEFT, y, line, COLOR_TITLE, TEXT_WIDTH);
    }

    snprintf(line, sizeof(line), "Start: %d workers, %d food, %d science",
             species->start_population, species->start_food, species->start_science);
    draw_text(TEXT_LEFT, PANEL_BOTTOM - 11, line, COLOR_TEXT_DIM, 1);
}

static void select_species(int index)
{
    draw_card_border(selected, false);
    selected = index;
    draw_card_border(selected, true);
    draw_details();
}

void new_game_start(void)
{
    m3_fill(COLOR_SKY);
    draw_text_centered(4, "CHOOSE YOUR SPECIES", COLOR_TITLE, 1);

    for (int i = 0; i < SPECIES_COUNT; i++) {
        draw_card(i);
        draw_card_border(i, false);
    }

    m3_frame(PANEL_LEFT, PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, COLOR_BUTTON_DISABLED_BORDER);

    draw_text(4, 151, "B: Back", COLOR_TEXT_DIM, 1);
    draw_text(PANEL_RIGHT - text_width("A: Start", 1), 151, "A: Start", COLOR_TEXT_DIM, 1);

    // Reopen on the last species picked
    selected = chosen_species;
    select_species(chosen_species);
}

Screen new_game_update(void)
{
    frames_on_screen++;

    if (key_hit(KEY_LEFT)) {
        select_species((selected + SPECIES_COUNT - 1) % SPECIES_COUNT);
    }
    if (key_hit(KEY_RIGHT)) {
        select_species((selected + 1) % SPECIES_COUNT);
    }
    if (key_hit(KEY_A | KEY_START)) {
        // How long the player took to choose is different every time, so it makes a good random seed
        random_seed(frames_on_screen);
        chosen_species = selected;
        game_new();
        return SCREEN_GAME;
    }
    if (key_hit(KEY_B)) {
        return SCREEN_MENU;
    }
    return SCREEN_NEW_GAME;
}
