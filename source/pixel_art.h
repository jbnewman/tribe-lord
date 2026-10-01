// Draws pixel art from a list of strings. Each character is one pixel and '.' is see-through.
#ifndef PIXEL_ART_H
#define PIXEL_ART_H

#include <tonc.h>

// Maps one character in the art to a color, e.g. { 's', HEX_COLOR(0xf4c27a) }.
typedef struct {
    char key;
    COLOR color;
} PaletteEntry;

// The palette list must end with an empty entry { 0 }.
void draw_pixel_art(int x, int y, const char *const rows[], int row_count, const PaletteEntry palette[], int scale);

#endif
