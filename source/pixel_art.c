#include "pixel_art.h"

void draw_pixel_art(int x, int y, const char *const rows[], int row_count, const PaletteEntry palette[], int scale)
{
    for (int row = 0; row < row_count; row++) {
        for (int col = 0; rows[row][col] != '\0'; col++) {
            char key = rows[row][col];

            for (int i = 0; palette[i].key != 0; i++) {
                if (palette[i].key == key) {
                    int left = x + col * scale;
                    int top = y + row * scale;
                    if (scale == 1) {
                        m3_plot(left, top, palette[i].color);
                    } else {
                        m3_rect(left, top, left + scale, top + scale, palette[i].color);
                    }
                    break;
                }
            }
        }
    }
}
