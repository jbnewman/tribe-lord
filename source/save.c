#include <stddef.h>
#include <string.h>
#include "save.h"
#include "game_state.h"

#define SAVE_MAGIC   0x44524C54     // "TLRD" marks memory written by this game
#define SAVE_VERSION 2              // bump when GameState changes so old saves are ignored

// SRAM can only be read and written one byte at a time
#define SAVE_MEMORY ((volatile u8 *)0x0E000000)

// Emulators and flash carts search the ROM for this text to know the game uses SRAM
__attribute__((used, aligned(4))) static const char SAVE_TYPE_TAG[] = "SRAM_V113";

typedef struct {
    u32 magic;
    u32 version;
    u32 size;
    int species;
    GameState game;
    u32 checksum;
} SaveData;

// Catches saves that were cut off or corrupted
static u32 checksum(const SaveData *data)
{
    const u8 *bytes = (const u8 *)data;
    u32 sum = 0;

    for (u32 i = 0; i < offsetof(SaveData, checksum); i++) {
        sum = sum * 31 + bytes[i];
    }
    return sum;
}

static bool read_valid_save(SaveData *data)
{
    u8 *bytes = (u8 *)data;
    for (u32 i = 0; i < sizeof(SaveData); i++) {
        bytes[i] = SAVE_MEMORY[i];
    }

    return data->magic == SAVE_MAGIC
        && data->version == SAVE_VERSION
        && data->size == sizeof(SaveData)
        && data->checksum == checksum(data);
}

bool save_exists(void)
{
    // Read the tag once so the linker doesn't throw it away as unused
    (void)*(volatile const char *)SAVE_TYPE_TAG;

    SaveData data;
    return read_valid_save(&data);
}

void save_game(void)
{
    SaveData data;

    // Zero first so padding bytes are the same every time, keeping the checksum stable
    memset(&data, 0, sizeof(data));
    data.magic = SAVE_MAGIC;
    data.version = SAVE_VERSION;
    data.size = sizeof(SaveData);
    data.species = chosen_species;
    data.game = game;
    data.checksum = checksum(&data);

    const u8 *bytes = (const u8 *)&data;
    for (u32 i = 0; i < sizeof(SaveData); i++) {
        SAVE_MEMORY[i] = bytes[i];
    }
}

bool load_game(void)
{
    SaveData data;
    if (!read_valid_save(&data)) {
        return false;
    }

    chosen_species = data.species;
    game = data.game;
    return true;
}

void delete_save(void)
{
    for (u32 i = 0; i < sizeof(u32); i++) {
        SAVE_MEMORY[i] = 0;
    }
}
