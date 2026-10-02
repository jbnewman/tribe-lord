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

## Release status

### v0.2.0: tribe lifecycle and management systems

This build is the current v0.2.0 implementation pass for Tribe Lord. It focuses on making the tribe feel alive without turning the game into a full life-sim. The core loop now includes villager age progression, births, deaths, job assignment, and a readable yearly report.

#### Included in the current build

- [x] A live villager roster with age, life stage, gender, fertility, hardiness, and job assignment.
- [x] Species-specific life cycles for humanoids, insectoids, and reptilians.
- [x] Yearly age progression and life-stage transitions.
- [x] Fertility checks and yearly births that grow the tribe.
- [x] Mortality checks that remove villagers and update population totals.
- [x] Event log entries for births, deaths, and major tribe events.
- [x] A compact yearly summary screen showing births, deaths, and population change.
- [x] Management panel logic for assigning villagers to farming, science, building, gathering, and defense roles.
- [x] Job-to-resource payoff balancing through food, science, housing, and growth bonuses.
- [x] Seasonal and winter pressure tied to food upkeep and yield changes.
- [x] Main menu info screens explaining the systems, jobs, and species differences.

#### Gameplay focus for this release

- Keep the village simulation readable on GBA-sized screens.
- Make population growth feel meaningful but not exponential.
- Let the player feel the tribe as a living system, not just a score counter.
- Keep formulas simple and easy to tune for future balancing passes.

#### Near-term follow-up ideas

- Add more variation in tribal events and random historical flavor.
- Tune fertility, mortality, and food pressure further for a smoother feel.
- Expand the job system if the current council of roles feels too small.
- Add deeper civilization progression beyond the current age-up loop.

#### Long-term direction

- More specialized village roles and deeper strategic planning.
- Family lineage, morale, or stability systems.
- Expanded planet/space-age progression and larger endgame goals.

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
