# Tribe Lord

A pixel art management sim for the Game Boy Advance, built with AI.

You are an alien overlord. Guide a small tribe of pixel workers from the Prehistoric Age to the Space Age, until they can launch into space to meet you.

## How to play

- Pick a species: Humanoids, Insectoids, or Reptilians, each with their own traits.
- Each turn your tribe gathers food and science from the tiles they've built.
- Buy tiles from three random offers and place them next to your land. Tiles earn bonuses on the right terrain (a Farm next to a river, a Temple on hills), so placement matters.
- Food grows your population, and every two workers eat one food per turn. Winter comes every 8 turns and halves your harvest, so stock up.
- Random events ask you to choose between two options.
- Spend science to advance to the next age, unlocking new tiles. Reach the Space Age to win.

| Button | Action |
|---|---|
| D-pad | Move cursor |
| L / R | Choose a tile card |
| A | Buy and place the card |
| SELECT | Advance to the next age |
| START | End turn |
| B | Save and quit |

The game autosaves at the end of every turn.

## Building

The game is written in C with [libtonc](https://github.com/devkitPro/libtonc) and built with devkitARM.

1. Install the tools:
   - **Windows:** in an Administrator PowerShell, run `powershell -ExecutionPolicy Bypass -File .\install-windows.ps1`
   - **macOS:** run `bash install-mac.sh`
2. Build:
   - **Windows:** `.\build.ps1`
   - **macOS:** `make`

This creates `tribe-lord.gba`, which runs in any GBA emulator such as [mGBA](https://mgba.io) or on real hardware with a flash cart.

## Web emulator

To play in a browser while developing, run:

```
node emulator/server.js
```

Then open http://localhost:8080. The first run downloads the mGBA web emulator. The page reloads the game automatically whenever you rebuild.

## Project layout

| Path | What's inside |
|---|---|
| `source/` | The game. Each screen has its own file (`menu.c`, `new_game.c`, `game.c`); game rules are in `game_state.c`. |
| `source/tiles.c`, `ages.c`, `events.c`, `terrain.c`, `species_data.c` | Game content. Add entries here to grow the game. |
| `emulator/` | Local web emulator for testing. |
| `install-*.ps1`, `install-*.sh` | Toolchain installers. |
