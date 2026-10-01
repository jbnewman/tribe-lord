// A 5x7 pixel font, the same one the web version uses. Lowercase letters draw as uppercase.
#ifndef PIXEL_FONT_H
#define PIXEL_FONT_H

#include <tonc.h>

#define FONT_CELL_WIDTH   6
#define FONT_GLYPH_HEIGHT 7
#define FONT_LINE_HEIGHT  10

// scale must be a whole number: 1 = normal, 2 = double size, and so on.
void draw_text(int x, int y, const char *text, COLOR color, int scale);
void draw_text_centered(int y, const char *text, COLOR color, int scale);
int text_width(const char *text, int scale);

// Wraps text onto new lines at spaces. Returns the y position below the last line.
int draw_text_wrapped(int x, int y, const char *text, COLOR color, int max_width);

#endif
