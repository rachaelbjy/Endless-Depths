# Endless Depths

**Endless Depths** is a 2D procedural dungeon survival game written in C using [raylib](https://www.raylib.com/).

Each floor contains a newly generated maze, moving enemies, limited visibility, collectible powerups and a hidden exit. The objective is to survive for as long as possible and reach the highest floor.

The game includes local accounts, separate Basic and Timed modes, saved progress, per-player high scores, leaderboards and looping background music.

## Features

* Procedurally generated dungeon layout on every floor
* Limited torch-based visibility
* Basic and Timed game modes
* Increasing enemy count and movement speed
* Six different powerups
* Local account creation and login
* Separate progress for each player and game mode
* Continue from the last saved floor
* Per-player highest-floor records
* Top-ten Basic and Timed leaderboards
* Restart from Floor 1 or the current floor
* Built-in tutorial and game-over screens
* Looping background music with music controls
* Character, enemy, item and interface graphics drawn using raylib shapes

## Game Modes

### Basic Mode

Basic Mode has no time limit. The player can explore each floor at their own pace while avoiding enemies and searching for the exit.

### Timed Mode

Timed Mode begins with 60 seconds on Floor 1.

The timer resets whenever the player enters a new floor:

```text
60 seconds + 15 seconds for each completed floor
```

For example, Floor 2 begins with 75 seconds and Floor 3 begins with 90 seconds.

The Freeze powerup pauses both enemy movement and the countdown timer.

## How to Play

1. Log in with an existing local account or create a new one.
2. Select Basic or Timed mode from the main menu.
3. Start a new game or continue from the saved floor.
4. Explore the visible parts of the dungeon.
5. Avoid enemies and collect useful powerups.
6. Find the purple exit door to enter the next floor.
7. Continue climbing until the player loses all health or runs out of time.

The player begins each run with:

* 100 HP
* A torch radius of 2.5 tiles
* A starting position near the centre of the map

Each enemy hit removes 15 HP. Health carries over when entering a new floor, but the torch radius and temporary powerups are reset.

Higher floors contain more enemies, faster enemy movement and denser maze layouts.

## Controls

| Control             | Action                                        |
| ------------------- | --------------------------------------------- |
| `W`, `A`, `S`, `D`  | Move the player                               |
| Arrow keys          | Move the player                               |
| Hold a movement key | Continue moving tile by tile                  |
| `M`                 | Save the current floor and return to the menu |
| `R`                 | Open the restart options                      |
| `P`                 | Toggle music during gameplay                  |
| Mouse               | Select buttons and account fields             |

A single key press moves the player by one tile. Holding the key begins repeated movement after a short delay.

## Powerups

| Powerup      | Effect                                                               |
| ------------ | -------------------------------------------------------------------- |
| Health       | Restores 20 HP, up to a maximum of 100                               |
| Torch        | Permanently increases the vision radius by 0.5 for the current floor |
| Freeze       | Stops enemies for 5 seconds and pauses the Timed Mode countdown      |
| Invisibility | Prevents enemies from damaging the player for 5 seconds              |
| Sword        | Allows the player to defeat enemies on contact for 5 seconds         |
| Lantern      | Reveals the entire dungeon for 2 seconds                             |

Health and Torch powerups are available from the beginning.

Other powerups are gradually introduced:

* Freeze from Floor 2
* Invisibility from Floor 3
* Sword from Floor 4
* Lantern from Floor 5

The number of available powerups increases gradually on higher floors.

## Enemies and Difficulty

Enemies move randomly through walkable dungeon tiles.

They cannot move through walls or the exit tile. When an enemy reaches the player, the player loses 15 HP unless Invisibility or Sword is active.

Difficulty increases with each floor:

* More enemies are spawned
* Enemies move more frequently
* The maze contains fewer open tiles
* The exit becomes harder to locate

The game supports up to 50 active enemies.

## Saving and Accounts

Game data is stored locally in text files.

### `accounts.txt`

Stores local usernames and passwords:

```text
username password
```

This account system is intended only for a local school project. Passwords are stored as plain text, so real passwords should not be used.

Usernames and passwords cannot contain spaces and may contain up to 31 characters.

### `lastfloor.txt`

Stores the floor shown by the Continue button for each player and mode:

```text
username MODE floor
```

Progress is saved when the player:

* Enters a new floor
* Returns to the menu
* Reaches the game-over screen
* Closes the game during a run

Continue restores the saved floor with a newly generated dungeon, full health and reset powerups. It does not restore the exact previous map or player position.

### `player_highscores.txt`

Stores each player's highest floor in Basic and Timed mode.

Only the best result for each player and mode is kept.

### `leaderboard.txt`

Stores the records used by the leaderboard screen.

Basic and Timed scores are displayed separately. Duplicate entries are merged, and only each player's highest floor for the selected mode is shown. The leaderboard displays up to ten players.

The included text files may contain sample account and score data. Their contents can be cleared to begin with an empty save system.

## Music

The game loads its background music from:

```text
resources/music/bgm.mp3
```

The music loops continuously while the game is running.

On the login and main menu screens, music can be controlled using the on-screen music button. During gameplay, press `P` to toggle it.

The game can still run without the audio file. If the file is missing, a message is displayed on the login and menu screens.

## Project Structure

```text
Endless-Depths/
├── main.c
├── main.exe
├── Makefile
├── Makefile.Android
├── main.code-workspace
├── accounts.txt
├── lastfloor.txt
├── leaderboard.txt
├── player_highscores.txt
├── LICENSE
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

### Main files

| File                  | Purpose                                                                                        |
| --------------------- | ---------------------------------------------------------------------------------------------- |
| `main.c`              | Contains the complete game logic, drawing, account system, saving system and screen management |
| `main.exe`            | Precompiled Windows executable                                                                 |
| `Makefile`            | Desktop build configuration for raylib                                                         |
| `Makefile.Android`    | Android-oriented raylib build configuration                                                    |
| `main.code-workspace` | VS Code workspace file                                                                         |
| `LICENSE`             | Repository licence information                                                                 |

### Save and resource files

| File                      | Purpose                                         |
| ------------------------- | ----------------------------------------------- |
| `accounts.txt`            | Stores local account usernames and passwords    |
| `lastfloor.txt`           | Stores each player's Continue floor             |
| `player_highscores.txt`   | Stores each player's highest floor in each mode |
| `leaderboard.txt`         | Stores records used by the leaderboard          |
| `resources/music/bgm.mp3` | Looping background music                        |

### VS Code configuration

| File                            | Purpose                                                                                                  |
| ------------------------------- | -------------------------------------------------------------------------------------------------------- |
| `.vscode/c_cpp_properties.json` | Compiler paths, raylib include paths, C standards and IntelliSense settings for Windows, macOS and Linux |
| `.vscode/launch.json`           | Debug and release launch configurations using GDB or LLDB                                                |
| `.vscode/tasks.json`            | Debug, release and active-file build tasks                                                               |
| `.vscode/settings.json`         | Hides generated object files, executables and version-control folders from the VS Code Explorer          |

## Requirements

To build the game from source, install:

* A C compiler supporting C99 or C11
* raylib
* GNU Make or MinGW Make
* Git, if cloning the repository
* Visual Studio Code with the Microsoft C/C++ extension, if using the included editor configuration

The supplied Windows configuration expects raylib and its compiler tools at:

```text
C:/raylib/raylib
C:/raylib/w64devkit
```

Update the configuration files if raylib is installed somewhere else.

## Building on Windows

Open the repository folder or `main.code-workspace` in Visual Studio Code.

Use:

```text
Ctrl + Shift + B
```

Then select either:

```text
build debug
```

or:

```text
build release
```

The configured Windows build task uses `mingw32-make.exe`.

The release build can also be started manually from the project directory:

```bash
mingw32-make RAYLIB_PATH=C:/raylib/raylib PROJECT_NAME=main OBJS=main.c
```

After building, run:

```bash
main.exe
```

A precompiled `main.exe` is also included. It must be run from the project directory so that the game can find the save files and `resources/music/bgm.mp3`.

## Building on macOS or Linux

Install raylib, a C compiler and Make.

Replace the `<path_to_raylib>` placeholders inside:

```text
.vscode/c_cpp_properties.json
.vscode/tasks.json
```

Then build from the project directory:

```bash
make RAYLIB_PATH=/path/to/raylib PROJECT_NAME=main OBJS=main.c PLATFORM=PLATFORM_DESKTOP
```

Run the compiled program:

```bash
./main
```

The included VS Code launch configurations use LLDB on macOS and GDB on Linux.

## Android Build File

`Makefile.Android` is included for projects using raylib's Android toolchain.

The current source code and VS Code tasks are configured primarily for desktop builds. Building for Android requires the Android SDK, Android NDK and the raylib Android project setup.

## Technical Details

* Language: C
* Graphics and audio library: raylib
* Window size: 960 × 760
* Target frame rate: 60 FPS
* Dungeon size: 32 × 24 tiles
* Tile size: 30 pixels
* Maximum enemies: 50
* Maximum leaderboard entries displayed: 10
* Map generation: random-walk dungeon carving
* Progress storage: local text files
* Graphics: raylib drawing primitives
* Audio format: MP3

The game uses a screen-based state system for:

* Login and account creation
* Main menu
* Tutorial
* Leaderboard
* Gameplay
* Restart selection
* Game over

## Important Note

This project uses a simple local text-file account system and is not intended for online authentication or the storage of sensitive information.

The game must have permission to read and write files in its working directory for accounts, progress, high scores and leaderboard records to work correctly.

## Author

Developed by **Rachael Bong**.

## Licence

See the included [`LICENSE`](LICENSE) file for the repository's licence terms.
