// Shared settings used by every screen in Tribe Lord.
#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <tonc.h>

#define GAME_VERSION "v0.2.0"

// Converts a web-style 0xRRGGBB color into the GBA's 15-bit color (5 bits each for red, green, blue).
#define HEX_COLOR(hex) ((((hex) >> 19) & 31) | ((((hex) >> 11) & 31) << 5) | ((((hex) >> 3) & 31) << 10))

#define COLOR_SKY                    HEX_COLOR(0x0b0b1e)
#define COLOR_SKY_LIGHT              HEX_COLOR(0x16163a)

#define COLOR_TEXT                   HEX_COLOR(0xf4e9c1)
#define COLOR_TEXT_DIM               HEX_COLOR(0x6b6b80)
#define COLOR_TEXT_SHADOW            HEX_COLOR(0x1a1030)
#define COLOR_TITLE                  HEX_COLOR(0x7cfc5a)

#define COLOR_BUTTON_FILL            HEX_COLOR(0x2a2440)
#define COLOR_BUTTON_BORDER          HEX_COLOR(0x7cfc5a)
#define COLOR_BUTTON_DISABLED_FILL   HEX_COLOR(0x1c1a28)
#define COLOR_BUTTON_DISABLED_BORDER HEX_COLOR(0x3a3850)

#endif
