# Endless Depths

Endless Depths is a 2D procedural dungeon survival game written in C using raylib.

The player explores randomly generated dungeon floors, avoids enemies, collects powerups and searches for the exit. Each new floor becomes more difficult, and the goal is to reach the highest floor possible.

## Features

* Randomly generated dungeon on every floor
* Basic and Timed game modes
* Local account creation and login
* Continue from the last saved floor
* Separate high scores for each player and mode
* Basic and Timed leaderboards
* Limited player vision
* Increasing enemy count and movement speed
* Six different powerups
* Restart from Floor 1 or the current floor
* Tutorial, leaderboard and game-over screens
* Background music with music controls

## Game Modes

### Basic Mode

No time limit. The player can explore each floor freely while avoiding enemies and looking for the exit.

### Timed Mode

Floor 1 starts with 60 seconds. Each new floor receives:

```text
60 + (floor - 1) × 15 seconds
```

If the timer reaches zero, the run ends.

## Controls

| Key        | Action                                |
| ---------- | ------------------------------------- |
| `W A S D`  | Move                                  |
| Arrow Keys | Move                                  |
| `M`        | Save current floor and return to menu |
| `R`        | Open restart options                  |
| `P`        | Toggle music during gameplay          |
| Mouse      | Use menus, buttons and login fields   |

Pressing a movement key moves one tile. Holding it continues moving tile by tile after a short delay.

## Powerups

| Powerup      | Effect                                                             |
| ------------ | ------------------------------------------------------------------ |
| Health       | Restores 20 HP, up to 100                                          |
| Torch        | Increases vision radius by 0.5                                     |
| Freeze       | Freezes enemies for 5 seconds and also pauses the Timed Mode timer |
| Invisibility | Prevents enemy damage for 5 seconds                                |
| Sword        | Allows enemies to be defeated on contact for 5 seconds             |
| Lantern      | Reveals the full dungeon for 2 seconds                             |

Freeze appears from Floor 2, Invisibility from Floor 3, Sword from Floor 4 and Lantern from Floor 5. More powerups gradually appear as the player reaches higher floors.

## Gameplay

The player starts each run with 100 HP and a limited vision radius.

Enemies move around the dungeon and each successful hit removes 15 HP. Higher floors contain more enemies, faster enemy movement and more difficult maze layouts.

Reaching the purple exit moves the player to the next floor. Health carries over, while vision and temporary powerup effects reset.

The game supports up to 50 enemies and displays the top 10 leaderboard entries for each mode.

## Saving and Accounts

The game uses local text files to store player data.

* `accounts.txt` stores usernames and passwords.
* `lastfloor.txt` stores the floor used by the Continue option for each player and mode.
* `player_highscores.txt` stores each player's best Basic and Timed floor.
* `leaderboard.txt` stores the scores used for the Basic and Timed leaderboards.

Progress is saved when moving to a new floor, returning to the menu, reaching game over or closing the game during a run.

The account system is intended only for this local project. Passwords are stored as plain text, so real passwords should not be used.

## Project Structure

```text
Endless-Depths/
│
├── main.c
├── main.exe
├── main.code-workspace
├── Makefile
├── Makefile.Android
├── LICENSE
│
├── accounts.txt
├── lastfloor.txt
├── leaderboard.txt
├── player_highscores.txt
│
├── resources/
│   └── music/
│       └── bgm.mp3
│
└── .vscode/
    ├── c_cpp_properties.json
    ├── launch.json
    ├── settings.json
    └── tasks.json
```

## Files

| File                    | Purpose                                                                                                                        |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| `main.c`                | Main source file containing the game logic, UI, dungeon generation, enemies, powerups, accounts, saving and leaderboard system |
| `main.exe`              | Compiled Windows version of the game                                                                                           |
| `main.code-workspace`   | VS Code workspace file                                                                                                         |
| `Makefile`              | Build configuration for the project                                                                                            |
| `Makefile.Android`      | Android build configuration                                                                                                    |
| `LICENSE`               | Licence information for the repository                                                                                         |
| `accounts.txt`          | Stores local usernames and passwords                                                                                           |
| `lastfloor.txt`         | Stores each player's latest Continue floor                                                                                     |
| `leaderboard.txt`       | Stores leaderboard results                                                                                                     |
| `player_highscores.txt` | Stores each player's highest floor for each mode                                                                               |
| `bgm.mp3`               | Background music used by the game                                                                                              |
| `c_cpp_properties.json` | C/C++ compiler, raylib include and IntelliSense settings                                                                       |
| `launch.json`           | VS Code Debug and Run configurations                                                                                           |
| `settings.json`         | VS Code project file visibility settings                                                                                       |
| `tasks.json`            | VS Code build tasks for debug and release builds                                                                               |

## Running the Game

### Windows

1. Download or clone the repository.
2. Keep all files in their original folders.
3. Run `main.exe`.

The game reads its save files from the project folder and loads background music from:

```text
resources/music/bgm.mp3
```

### Building from Source

To build the project yourself, install:

* C compiler
* raylib
* Make or MinGW Make

The included VS Code configuration uses:

```text
C:/raylib/raylib
C:/raylib/w64devkit
```

If raylib is installed elsewhere, update the paths in the VS Code configuration files.

In VS Code, use the included `build debug` or `build release` task to compile the game.

## Technical Details

* Language: C
* Library: raylib
* Window: 960 × 760
* Target frame rate: 60 FPS
* Dungeon: 32 × 24 tiles
* Tile size: 30 pixels
* Maximum enemies: 50
* Game data: local text files
* Graphics: raylib drawing functions
* Music: MP3

## Author

Rachael Bong

## License

See the `LICENSE` file for licence information.
