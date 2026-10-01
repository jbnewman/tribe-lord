// The list of screens. Each screen has a *_start() function that draws it
// and an *_update() function that runs every frame and returns which screen comes next.
#ifndef SCREENS_H
#define SCREENS_H

typedef enum {
    SCREEN_MENU,
    SCREEN_NEW_GAME,
    SCREEN_GAME
} Screen;

#endif
