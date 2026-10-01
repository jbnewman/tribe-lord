// Saves the game in the cartridge's battery-backed memory (SRAM), so it survives turning the GBA off.
#ifndef SAVE_H
#define SAVE_H

#include <tonc.h>

bool save_exists(void);
void save_game(void);
bool load_game(void);
void delete_save(void);

#endif
