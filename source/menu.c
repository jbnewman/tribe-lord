#include <tonc.h>
#include "menu.h"
#include "game_config.h"
#include "pixel_font.h"
#include "random.h"
#include "save.h"

#define BUTTON_NEW_GAME 0
#define BUTTON_CONTINUE 1

#define STAR_COUNT    50
#define PLANET_X      120
#define PLANET_Y      290
#define PLANET_RADIUS 160
#define SHIP_X        200
#define SHIP_Y        16

#define COLOR_GRASS       HEX_COLOR(0x3d8a4a)
#define COLOR_GRASS_DARK  HEX_COLOR(0x2f6b3a)
#define COLOR_ATMOSPHERE  HEX_COLOR(0x2a5a8a)
#define COLOR_HUT         HEX_COLOR(0x8b5a2b)
#define COLOR_ROOF        HEX_COLOR(0xc9a24a)
#define COLOR_WORKER      HEX_COLOR(0xf4c27a)
#define COLOR_SHIP_DOME   HEX_COLOR(0x9be7ff)
#define COLOR_SHIP_HULL   HEX_COLOR(0x8a8fa8)
#define COLOR_SHIP_BELLY  HEX_COLOR(0x5c6078)
#define COLOR_SHIP_LIGHT  HEX_COLOR(0xffe066)
#define COLOR_STAR        HEX_COLOR(0xffffff)

typedef struct {
    int x;
    int y;
    bool bright;
} Star;

static Star stars[STAR_COUNT];
static int frame;
static bool has_save;
static int selected_button;

// Blends two colors: step 0 gives a, step == steps gives b
static COLOR mix_colors(COLOR a, COLOR b, int step, int steps)
{
    int red   = (a & 31)         + (((b & 31)         - (a & 31))         * step) / steps;
    int green = ((a >> 5) & 31)  + ((((b >> 5) & 31)  - ((a >> 5) & 31))  * step) / steps;
    int blue  = ((a >> 10) & 31) + ((((b >> 10) & 31) - ((a >> 10) & 31)) * step) / steps;
    return red | (green << 5) | (blue << 10);
}

static void draw_sky(void)
{
    int bands = 5;
    int band_height = SCREEN_HEIGHT / bands;

    for (int i = 0; i < bands; i++) {
        COLOR color = mix_colors(COLOR_SKY, COLOR_SKY_LIGHT, i, bands - 1);
        m3_rect(0, i * band_height, SCREEN_WIDTH, (i + 1) * band_height, color);
    }
}

// Stars stay out of these areas so twinkling never paints over text or the ship
static bool star_spot_is_free(int x, int y)
{
    bool in_title   = x >= 26 && x < 214 && y >= 36 && y < 80;
    bool in_buttons = x >= 72 && x < 168 && y >= 86 && y < 128;
    bool in_ship    = x >= 184 && x < 214 && y >= 8 && y < 24;
    return !in_title && !in_buttons && !in_ship;
}

static void draw_star(const Star *star)
{
    m3_plot(star->x, star->y, star->bright ? COLOR_STAR : COLOR_TEXT_DIM);
}

static void create_stars(void)
{
    for (int i = 0; i < STAR_COUNT; i++) {
        do {
            stars[i].x = random_range(0, SCREEN_WIDTH);
            stars[i].y = random_range(0, 118);
        } while (!star_spot_is_free(stars[i].x, stars[i].y));

        stars[i].bright = random_range(0, 2);
        draw_star(&stars[i]);
    }
}

static bool inside_planet(int x, int y, int radius)
{
    int dx = x - PLANET_X;
    int dy = y - PLANET_Y;
    return dx * dx + dy * dy <= radius * radius;
}

static void draw_planet(void)
{
    for (int y = PLANET_Y - PLANET_RADIUS - 3; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (inside_planet(x, y, PLANET_RADIUS - 4)) {
                m3_plot(x, y, COLOR_GRASS);
            } else if (inside_planet(x, y, PLANET_RADIUS)) {
                m3_plot(x, y, COLOR_GRASS_DARK);
            } else if (inside_planet(x, y, PLANET_RADIUS + 3)) {
                m3_plot(x, y, COLOR_ATMOSPHERE);
            }
        }
    }
}

// Y position of the planet's surface at a given x
static int surface_y(int x)
{
    int y = PLANET_Y - PLANET_RADIUS;
    while (!inside_planet(x, y, PLANET_RADIUS)) {
        y++;
    }
    return y;
}

static void draw_tribe(void)
{
    const int hut_offsets[] = { -40, -20, 30 };
    const int worker_offsets[] = { -30, 0, 14, 40 };

    for (int i = 0; i < 3; i++) {
        int x = PLANET_X + hut_offsets[i];
        int ground = surface_y(x);

        m3_rect(x - 3, ground - 4, x + 3, ground, COLOR_HUT);
        for (int row = 0; row < 4; row++) {
            m3_rect(x - 4 + row, ground - 5 - row, x + 4 - row, ground - 4 - row, COLOR_ROOF);
        }
    }

    for (int i = 0; i < 4; i++) {
        int x = PLANET_X + worker_offsets[i];
        m3_rect(x, surface_y(x) - 3, x + 1, surface_y(x), COLOR_WORKER);
    }
}

static void draw_ship(void)
{
    m3_rect(SHIP_X - 3,  SHIP_Y - 4, SHIP_X + 3,  SHIP_Y,     COLOR_SHIP_DOME);
    m3_rect(SHIP_X - 10, SHIP_Y,     SHIP_X + 10, SHIP_Y + 3, COLOR_SHIP_HULL);
    m3_rect(SHIP_X - 6,  SHIP_Y + 3, SHIP_X + 6,  SHIP_Y + 5, COLOR_SHIP_BELLY);
}

static void draw_ship_lights(int lit)
{
    for (int i = 0; i < 3; i++) {
        int x = SHIP_X - 6 + i * 5;
        m3_rect(x, SHIP_Y + 1, x + 2, SHIP_Y + 2, i == lit ? COLOR_SHIP_LIGHT : COLOR_SHIP_BELLY);
    }
}

static void draw_button(int y, const char *label, bool enabled, bool selected)
{
    m3_rect(75, y, 165, y + 16, enabled ? COLOR_BUTTON_FILL : COLOR_BUTTON_DISABLED_FILL);
    m3_frame(75, y, 165, y + 16, selected ? COLOR_BUTTON_BORDER : COLOR_BUTTON_DISABLED_BORDER);
    draw_text_centered(y + 5, label, enabled ? COLOR_TEXT : COLOR_TEXT_DIM, 1);
}

static void draw_buttons(void)
{
    draw_button(88, "NEW GAME", true, selected_button == BUTTON_NEW_GAME);
    draw_button(110, "CONTINUE", has_save, selected_button == BUTTON_CONTINUE);
}

void menu_start(void)
{
    frame = 0;

    draw_sky();
    create_stars();
    draw_planet();
    draw_tribe();
    draw_ship();
    draw_ship_lights(0);

    // Drop shadow first, then the title on top of it
    draw_text(34, 43, "TRIBE LORD", COLOR_TEXT_SHADOW, 3);
    draw_text(31, 40, "TRIBE LORD", COLOR_TITLE, 3);
    draw_text_centered(70, "Guide your tribe to the stars", COLOR_TEXT, 1);

    has_save = save_exists();
    selected_button = has_save ? BUTTON_CONTINUE : BUTTON_NEW_GAME;
    draw_buttons();

    draw_text(2, 151, "v0.1", COLOR_TEXT_DIM, 1);
}

Screen menu_update(void)
{
    frame++;

    if (frame % 6 == 0) {
        Star *star = &stars[random_range(0, STAR_COUNT)];
        star->bright = !star->bright;
        draw_star(star);
    }

    if (frame % 20 == 0) {
        draw_ship_lights((frame / 20) % 3);
    }

    if (has_save && key_hit(KEY_UP | KEY_DOWN)) {
        selected_button = selected_button == BUTTON_NEW_GAME ? BUTTON_CONTINUE : BUTTON_NEW_GAME;
        draw_buttons();
    }

    if (key_hit(KEY_A | KEY_START)) {
        if (selected_button == BUTTON_CONTINUE && load_game()) {
            return SCREEN_GAME;
        }
        return SCREEN_NEW_GAME;
    }
    return SCREEN_MENU;
}
