// Tribe Lord - Game Boy Advance version.
// The GBA runs this loop 60 times a second: wait for the screen to finish drawing,
// read the buttons, then let the current screen update itself.
#include <tonc.h>
#include "screens.h"
#include "menu.h"
#include "new_game.h"
#include "game.h"

static void start_screen(Screen screen)
{
    m3_fill(0);

    switch (screen) {
        case SCREEN_MENU:     menu_start();     break;
        case SCREEN_NEW_GAME: new_game_start(); break;
        case SCREEN_GAME:     game_start();     break;
    }
}

static Screen update_screen(Screen screen)
{
    switch (screen) {
        case SCREEN_MENU:     return menu_update();
        case SCREEN_NEW_GAME: return new_game_update();
        case SCREEN_GAME:     return game_update();
    }
    return screen;
}

int main(void)
{
    // VBlank interrupt lets VBlankIntrWait() sleep until each new frame
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    // Mode 3: the screen is a simple 240x160 grid of pixels we can draw on directly
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    Screen current = SCREEN_MENU;
    start_screen(current);

    while (1) {
        VBlankIntrWait();
        key_poll();

        Screen next = update_screen(current);
        if (next != current) {
            current = next;
            start_screen(current);
        }
    }
}
