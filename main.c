// ============================================================================
// Game Name: Endless Depths
// File Name: main.c
// Author: Rachael Bong
// Description: A 2D procedural dungeon survival game built with raylib.
// The player explores random floors, avoids enemies, collects powerups,
// saves account progress, and tries to reach the highest floor possible.
// ============================================================================

#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


// Gameplay tile colors tuned for clearer contrast.
#define FLOOR_COLOR ((Color){238, 238, 230, 255})
#define WALL_COLOR  ((Color){92, 92, 98, 255})

// ==========================================
// 1. Constants, Game States, and Data Structures
// ==========================================

typedef enum GameScreen {
    // Each value represents one full screen in the game.
    SCREEN_LOGIN,
    SCREEN_MENU,
    SCREEN_TUTORIAL,
    SCREEN_LEADERBOARD,
    SCREEN_GAMEPLAY,
    SCREEN_GAMEOVER,
    SCREEN_RESTART_CHOICE
} GameScreen;

// Basic mode has no countdown. Timed mode adds a timer challenge.
typedef enum GameMode { MODE_BASIC, MODE_TIMED } GameMode;

typedef enum TileType {
    // Every map cell stores one of these tile types.
    TILE_FLOOR,
    TILE_WALL,
    TILE_HEALTH_PICKUP,
    TILE_OIL_PICKUP,
    TILE_FREEZE_PICKUP,
    TILE_INVISIBLE_PICKUP,
    TILE_KILL_PICKUP,
    TILE_REVEAL_PICKUP,
    TILE_EXIT
} TileType;

// Map size, tile size, entity limits, and text input limits.
#define MAX_X 32
#define MAX_Y 24
#define TILE_SIZE 30
#define MAX_ENEMIES 50
#define MAX_NAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 32
#define MAX_LEADERBOARD 10

// Shared structure for the player and enemies.
typedef struct Entity {
    Vector2 position;
    int health;
    float torchRadius;
} Entity;

// Stores the full tile grid for one dungeon floor.
typedef struct MapData {
    TileType grid[MAX_X][MAX_Y];
} MapData;

// Stores one leaderboard or save-file record.
typedef struct LeaderboardEntry {
    char username[MAX_NAME_LENGTH];
    char mode[16];
    int floor;
} LeaderboardEntry;

// ==========================================
// 2. General Helper Functions
// ==========================================

// Draws readable text by placing a dark shadow behind the main text.
void DrawTextClear(const char *text, int x, int y, int fontSize, Color color) {
    DrawText(text, x + 2, y + 2, fontSize, BLACK);
    DrawText(text, x, y, fontSize, color);
}

// Draws text horizontally centered on the screen.
void DrawTextCentered(const char *text, int y, int fontSize, Color color) {
    int width = MeasureText(text, fontSize);
    int x = GetScreenWidth()/2 - width / 2;
    DrawTextClear(text, x, y, fontSize, color);
}

// Draws text centered inside a specific rectangle width.
void DrawTextBoxCentered(const char *text, int boxX, int boxY, int boxW, int fontSize, Color color) {
    int width = MeasureText(text, fontSize);
    DrawTextClear(text, boxX + boxW/2 - width/2, boxY, fontSize, color);
}

// Returns true only on the frame when the left mouse button clicks inside a rectangle.
bool IsButtonClicked(Rectangle button) {
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), button);
}

// Draws a consistent clickable UI button with hover feedback.
void DrawUIButton(Rectangle button, const char *text, int fontSize, Color borderColor, Color textColor) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, button);
    Color fill = hover ? (Color){45, 45, 55, 255} : (Color){22, 22, 30, 255};
    DrawRectangleRounded(button, 0.18f, 8, fill);
    DrawRectangleRoundedLines(button, 0.18f, 8, borderColor);
    int textW = MeasureText(text, fontSize);
    DrawTextClear(text, (int)(button.x + button.width/2 - textW/2), (int)(button.y + button.height/2 - fontSize/2), fontSize, textColor);
}

// Calculates the bottom-center music button rectangle.
Rectangle GetMusicButton(void) {
    // Bottom-center music button for non-gameplay screens.
    // Gameplay uses keyboard P only.
    int w = 126;
    int h = 34;
    return (Rectangle){ GetScreenWidth()/2 - w/2, GetScreenHeight() - 62, w, h };
}

// Draws the music toggle button using the current music state.
void DrawMusicButton(bool musicEnabled) {
    DrawUIButton(GetMusicButton(), musicEnabled ? "Music: ON" : "Music: OFF", 16, musicEnabled ? SKYBLUE : RED, WHITE);
}

// Controls which screens show the mouse music button.
bool ShouldShowMusicMouseButton(GameScreen screen) {
    return screen == SCREEN_LOGIN || screen == SCREEN_MENU;
}

// Checks whether a file exists before trying to load it.
bool FileExistsSimple(const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (file != NULL) {
        fclose(file);
        return true;
    }
    return false;
}

// Checks whether a tile coordinate is inside the dungeon grid.
bool IsInsideMap(int x, int y) {
    return x >= 0 && x < MAX_X && y >= 0 && y < MAX_Y;
}

// Checks whether text represents a valid game mode.
bool IsModeText(const char *text) {
    return strcmp(text, "BASIC") == 0 || strcmp(text, "TIMED") == 0 ||
           strcmp(text, "Basic") == 0 || strcmp(text, "Timed") == 0 ||
           strcmp(text, "basic") == 0 || strcmp(text, "timed") == 0;
}

// Converts different mode spellings into the standard save-file spelling.
void NormalizeModeText(const char *input, char *output) {
    if (strcmp(input, "TIMED") == 0 || strcmp(input, "Timed") == 0 || strcmp(input, "timed") == 0) {
        strcpy(output, "TIMED");
    } else {
        strcpy(output, "BASIC");
    }
}

// Checks whether a string contains digits only.
bool IsNumberText(const char *text) {
    if (text[0] == '\0') return false;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') return false;
    }
    return true;
}

// Reads one progress record from a save-file line.
bool ParseProgressRecord(const char *line, char *username, char *mode, int *floor) {
    char a[MAX_NAME_LENGTH];
    char b[MAX_NAME_LENGTH];
    char c[MAX_NAME_LENGTH];

    if (sscanf(line, "%31s %31s %31s", a, b, c) != 3) return false;

    // Current format: username MODE floor
    if (IsModeText(b) && IsNumberText(c)) {
        strcpy(username, a);
        NormalizeModeText(b, mode);
        *floor = atoi(c);
        return true;
    }

    // Supported format: username floor MODE
    if (IsNumberText(b) && IsModeText(c)) {
        strcpy(username, a);
        NormalizeModeText(c, mode);
        *floor = atoi(b);
        return true;
    }

    // Supported format: MODE username floor
    if (IsModeText(a) && IsNumberText(c)) {
        strcpy(username, b);
        NormalizeModeText(a, mode);
        *floor = atoi(c);
        return true;
    }

    return false;
}

// Finds the best saved floor for one player and one mode inside a file.
int ScanBestFloorInFile(const char *fileName, const char *username, GameMode mode, bool ignoreFloorOne) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) return 0;

    char line[160];
    char savedUser[MAX_NAME_LENGTH];
    char savedMode[16];
    int savedFloor;
    int bestFloor = 0;
    const char *wantedMode = mode == MODE_BASIC ? "BASIC" : "TIMED";

    while (fgets(line, sizeof(line), file) != NULL) {
        if (ParseProgressRecord(line, savedUser, savedMode, &savedFloor)) {
            if (strcmp(savedUser, username) == 0 && strcmp(savedMode, wantedMode) == 0) {
                if (!ignoreFloorOne || savedFloor > 1) {
                    if (savedFloor > bestFloor) bestFloor = savedFloor;
                }
            }
        }
    }

    fclose(file);
    return bestFloor;
}


// Loads one player's highest floor for the selected mode.
int LoadPlayerHighScore(const char *username, GameMode mode) {
    int bestFloor = 0;
    int candidate;

    // Main per-player high score file.
    candidate = ScanBestFloorInFile("player_highscores.txt", username, mode, false);
    if (candidate > bestFloor) bestFloor = candidate;

    // Leaderboard can also provide a valid best floor for the player.
    candidate = ScanBestFloorInFile("leaderboard.txt", username, mode, false);
    if (candidate > bestFloor) bestFloor = candidate;

    // Continue records can also recover a real progress value.
    // Floor 1 is ignored here because it may only be the default starting floor.
    candidate = ScanBestFloorInFile("lastfloor.txt", username, mode, true);
    if (candidate > bestFloor) bestFloor = candidate;

    return bestFloor;
}

// Loads both Basic and Timed highest floors for one player.
void LoadPlayerHighScores(const char *username, int *basicScore, int *timedScore) {
    *basicScore = LoadPlayerHighScore(username, MODE_BASIC);
    *timedScore = LoadPlayerHighScore(username, MODE_TIMED);
}

// Saves one player's best floor for one mode, keeping only the best value.
void SavePlayerHighScore(const char *username, GameMode mode, int floor) {
    LeaderboardEntry entries[400];
    int count = 0;
    bool recordFound = false;
    const char *modeText = mode == MODE_BASIC ? "BASIC" : "TIMED";

    FILE *file = fopen("player_highscores.txt", "r");
    if (file != NULL) {
        char savedUser[MAX_NAME_LENGTH];
        char savedMode[16];
        int savedFloor;

        while (fscanf(file, "%31s %15s %d", savedUser, savedMode, &savedFloor) == 3 && count < 400) {
            if (strcmp(savedUser, username) == 0 && strcmp(savedMode, modeText) == 0) {
                if (floor > savedFloor) savedFloor = floor;
                recordFound = true;
            }

            bool merged = false;
            for (int i = 0; i < count; i++) {
                if (strcmp(entries[i].username, savedUser) == 0 && strcmp(entries[i].mode, savedMode) == 0) {
                    if (savedFloor > entries[i].floor) entries[i].floor = savedFloor;
                    merged = true;
                    break;
                }
            }

            if (!merged) {
                strcpy(entries[count].username, savedUser);
                strcpy(entries[count].mode, savedMode);
                entries[count].floor = savedFloor;
                count++;
            }
        }
        fclose(file);
    }

    if (!recordFound && count < 400) {
        strcpy(entries[count].username, username);
        strcpy(entries[count].mode, modeText);
        entries[count].floor = floor;
        count++;
    }

    file = fopen("player_highscores.txt", "w");
    if (file != NULL) {
        for (int i = 0; i < count; i++) {
            fprintf(file, "%s %s %d\n", entries[i].username, entries[i].mode, entries[i].floor);
        }
        fclose(file);
    }
}


// Loads the floor used by the Continue button for one player and one mode.
int LoadLastFloor(const char *username, GameMode mode) {
    FILE *file = fopen("lastfloor.txt", "r");
    char savedUser[MAX_NAME_LENGTH];
    char savedMode[16];
    int savedFloor;
    const char *modeText = mode == MODE_BASIC ? "BASIC" : "TIMED";
    int result = 0;

    if (file != NULL) {
        while (fscanf(file, "%31s %15s %d", savedUser, savedMode, &savedFloor) == 3) {
            if (strcmp(savedUser, username) == 0 && strcmp(savedMode, modeText) == 0) {
                result = savedFloor;
            }
        }

        fclose(file);
    }

    // If this account has a saved best floor but no Continue record yet,
    // let Continue start from that player's own progress instead of showing Floor 1/None.
    if (result <= 0) {
        int bestFloor = LoadPlayerHighScore(username, mode);
        if (bestFloor > 0) result = bestFloor;
    }

    return result;
}

// Saves the floor used by the Continue button for one player and one mode.
void SaveLastFloor(const char *username, GameMode mode, int floor) {
    LeaderboardEntry entries[400];
    int count = 0;
    bool recordFound = false;
    const char *modeText = mode == MODE_BASIC ? "BASIC" : "TIMED";

    FILE *file = fopen("lastfloor.txt", "r");
    if (file != NULL) {
        char savedUser[MAX_NAME_LENGTH];
        char savedMode[16];
        int savedFloor;

        while (fscanf(file, "%31s %15s %d", savedUser, savedMode, &savedFloor) == 3 && count < 400) {
            if (strcmp(savedUser, username) == 0 && strcmp(savedMode, modeText) == 0) {
                savedFloor = floor;
                recordFound = true;
            }

            bool merged = false;
            for (int i = 0; i < count; i++) {
                if (strcmp(entries[i].username, savedUser) == 0 && strcmp(entries[i].mode, savedMode) == 0) {
                    entries[i].floor = savedFloor;
                    merged = true;
                    break;
                }
            }

            if (!merged) {
                strcpy(entries[count].username, savedUser);
                strcpy(entries[count].mode, savedMode);
                entries[count].floor = savedFloor;
                count++;
            }
        }
        fclose(file);
    }

    if (!recordFound && count < 400) {
        strcpy(entries[count].username, username);
        strcpy(entries[count].mode, modeText);
        entries[count].floor = floor;
        count++;
    }

    file = fopen("lastfloor.txt", "w");
    if (file != NULL) {
        for (int i = 0; i < count; i++) {
            fprintf(file, "%s %s %d\n", entries[i].username, entries[i].mode, entries[i].floor);
        }
        fclose(file);
    }
}

// Makes sure a logged-in player has Continue records for both modes.
void EnsureContinueFloorsForUser(const char *username) {
    // Make Continue usable immediately after opening/logging in.
    // If a mode has no saved floor yet, default it to Floor 1.
    if (LoadLastFloor(username, MODE_BASIC) <= 0) SaveLastFloor(username, MODE_BASIC, 1);
    if (LoadLastFloor(username, MODE_TIMED) <= 0) SaveLastFloor(username, MODE_TIMED, 1);
}


// Simple account system.
// Note: This is for a school project / local game only.
// Passwords are stored as plain text, so do not use real passwords.
// Checks whether a username already exists in the local account file.
bool AccountExists(const char *username) {
    FILE *file = fopen("accounts.txt", "r");
    if (file == NULL) return false;

    char savedUser[MAX_NAME_LENGTH];
    char savedPass[MAX_PASSWORD_LENGTH];

    while (fscanf(file, "%31s %31s", savedUser, savedPass) == 2) {
        if (strcmp(savedUser, username) == 0) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

// Checks whether the username and password match a saved local account.
bool ValidateLogin(const char *username, const char *password) {
    FILE *file = fopen("accounts.txt", "r");
    if (file == NULL) return false;

    char savedUser[MAX_NAME_LENGTH];
    char savedPass[MAX_PASSWORD_LENGTH];

    while (fscanf(file, "%31s %31s", savedUser, savedPass) == 2) {
        if (strcmp(savedUser, username) == 0 && strcmp(savedPass, password) == 0) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

// Creates a local account when the username is not already taken.
bool CreateAccount(const char *username, const char *password) {
    if (strlen(username) == 0 || strlen(password) == 0) return false;
    if (AccountExists(username)) return false;

    FILE *file = fopen("accounts.txt", "a");
    if (file == NULL) return false;

    fprintf(file, "%s %s\n", username, password);
    fclose(file);
    return true;
}

// Saves one leaderboard entry per player per mode, keeping the highest floor only.
void SaveLeaderboardScore(const char *username, GameMode mode, int floor) {
    // Save only one best record per player for each mode.
    LeaderboardEntry entries[400];
    int count = 0;
    bool recordFound = false;
    const char *modeText = mode == MODE_BASIC ? "BASIC" : "TIMED";

    FILE *file = fopen("leaderboard.txt", "r");
    if (file != NULL) {
        char savedUser[MAX_NAME_LENGTH];
        char savedMode[16];
        int savedFloor;

        while (fscanf(file, "%31s %15s %d", savedUser, savedMode, &savedFloor) == 3 && count < 400) {
            if (strcmp(savedUser, username) == 0 && strcmp(savedMode, modeText) == 0) {
                if (floor > savedFloor) savedFloor = floor;
                recordFound = true;
            }

            // Merge duplicate records so each player keeps one best score per mode.
            bool merged = false;
            for (int i = 0; i < count; i++) {
                if (strcmp(entries[i].username, savedUser) == 0 && strcmp(entries[i].mode, savedMode) == 0) {
                    if (savedFloor > entries[i].floor) entries[i].floor = savedFloor;
                    merged = true;
                    break;
                }
            }

            if (!merged) {
                strcpy(entries[count].username, savedUser);
                strcpy(entries[count].mode, savedMode);
                entries[count].floor = savedFloor;
                count++;
            }
        }
        fclose(file);
    }

    if (!recordFound && count < 400) {
        strcpy(entries[count].username, username);
        strcpy(entries[count].mode, modeText);
        entries[count].floor = floor;
        count++;
    }

    file = fopen("leaderboard.txt", "w");
    if (file != NULL) {
        for (int i = 0; i < count; i++) {
            fprintf(file, "%s %s %d\n", entries[i].username, entries[i].mode, entries[i].floor);
        }
        fclose(file);
    }
}


// Updates high score files, leaderboard files, and in-memory score values.
void UpdatePlayerBestFloor(const char *username, GameMode mode, int floor, int *basicScore, int *timedScore) {
    if (strcmp(username, "Guest") == 0) return;

    if (mode == MODE_BASIC) {
        if (floor > *basicScore) *basicScore = floor;
        SavePlayerHighScore(username, MODE_BASIC, *basicScore);
        SaveLeaderboardScore(username, MODE_BASIC, *basicScore);
    } else {
        if (floor > *timedScore) *timedScore = floor;
        SavePlayerHighScore(username, MODE_TIMED, *timedScore);
        SaveLeaderboardScore(username, MODE_TIMED, *timedScore);
    }
}


// Loads, merges, sorts, and limits leaderboard records for one mode.
int LoadLeaderboard(LeaderboardEntry entries[], GameMode mode) {
    FILE *file = fopen("leaderboard.txt", "r");
    if (file == NULL) return 0;

    int count = 0;
    char savedUser[MAX_NAME_LENGTH];
    char savedMode[16];
    int savedFloor;
    const char *wantedMode = mode == MODE_BASIC ? "BASIC" : "TIMED";

    while (fscanf(file, "%31s %15s %d", savedUser, savedMode, &savedFloor) == 3) {
        if (strcmp(savedMode, wantedMode) == 0) {
            // Keep only each player's highest floor for this mode.
            bool found = false;
            for (int i = 0; i < count; i++) {
                if (strcmp(entries[i].username, savedUser) == 0) {
                    if (savedFloor > entries[i].floor) entries[i].floor = savedFloor;
                    found = true;
                    break;
                }
            }

            if (!found && count < 200) {
                strcpy(entries[count].username, savedUser);
                strcpy(entries[count].mode, savedMode);
                entries[count].floor = savedFloor;
                count++;
            }
        }
    }
    fclose(file);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            bool shouldSwap = false;
            if (entries[j].floor > entries[i].floor) shouldSwap = true;
            else if (entries[j].floor == entries[i].floor && strcmp(entries[j].username, entries[i].username) < 0) shouldSwap = true;

            if (shouldSwap) {
                LeaderboardEntry temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    if (count > MAX_LEADERBOARD) count = MAX_LEADERBOARD;
    return count;
}

// Draws the player character directly with raylib shapes.
void DrawPlayerCharacter(int tileX, int tileY, int tileSize, bool invisible) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    Color shirt = invisible ? BLUE : SKYBLUE;

    // A built-in character drawn with shapes.
    DrawCircle(x + tileSize/2, y + tileSize/4, tileSize/5, BEIGE);                 // head
    DrawRectangle(x + tileSize/3, y + tileSize/2 - 2, tileSize/3, tileSize/3, shirt); // body
    DrawRectangle(x + tileSize/3 - 4, y + tileSize/2, 4, tileSize/4, BEIGE);         // left arm
    DrawRectangle(x + tileSize*2/3, y + tileSize/2, 4, tileSize/4, BEIGE);           // right arm
    DrawRectangle(x + tileSize/3, y + tileSize*5/6 - 2, 6, tileSize/6, DARKBLUE);    // left leg
    DrawRectangle(x + tileSize/2 + 2, y + tileSize*5/6 - 2, 6, tileSize/6, DARKBLUE);// right leg
    DrawRectangle(x + tileSize/3, y + tileSize/12, tileSize/3, 5, RED);             // cap
    DrawPixel(x + tileSize/2 + 3, y + tileSize/4 - 1, BLACK);                       // eye
}

// Draws the health recovery powerup icon.
void DrawHealthPowerupIcon(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    int cx = x + tileSize/2;
    int cy = y + tileSize/2;

    // Health icon: clean round medical badge with no extra highlight dot.
    Color deepGreen = (Color){ 18, 105, 48, 255 };
    Color fillGreen = (Color){ 82, 196, 95, 255 };
    Color plusColor = (Color){ 232, 240, 228, 255 };

    DrawCircle(cx, cy, tileSize/3 + 2, deepGreen);
    DrawCircle(cx, cy, tileSize/3 - 1, fillGreen);
    DrawRectangle(cx - 3, cy - 8, 6, 16, plusColor);
    DrawRectangle(cx - 8, cy - 3, 16, 6, plusColor);
}

// Draws the torch powerup icon for increasing vision radius.
void DrawOilPowerupIcon(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    int cx = x + tileSize/2;

    // Torch icon for vision radius.
    DrawRectangle(cx - 5, y + 12, 10, 15, DARKBROWN);
    DrawRectangle(cx - 3, y + 13, 6, 13, BROWN);
    DrawRectangle(cx - 8, y + 25, 16, 4, DARKBROWN);
    DrawCircle(cx, y + 9, 8, ORANGE);
    DrawCircle(cx, y + 7, 4, YELLOW);
    DrawTriangle((Vector2){cx, y + 1}, (Vector2){cx - 7, y + 12}, (Vector2){cx + 7, y + 12}, GOLD);
}

// Draws the freeze powerup icon.
void DrawFreezePowerupIcon(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    int cx = x + tileSize/2;
    int cy = y + tileSize/2;
    DrawCircle(cx, cy, tileSize/3 + 1, BLUE);
    DrawCircle(cx, cy, tileSize/3, SKYBLUE);
    DrawCircle(cx, cy, tileSize/5, RAYWHITE);
    DrawLine(cx, y + 6, cx, y + tileSize - 6, BLUE);
    DrawLine(x + 6, cy, x + tileSize - 6, cy, BLUE);
    DrawLine(x + 9, y + 9, x + tileSize - 9, y + tileSize - 9, BLUE);
    DrawLine(x + tileSize - 9, y + 9, x + 9, y + tileSize - 9, BLUE);
}

// Draws the invisibility powerup icon.
void DrawInvisiblePowerupIcon(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    int cx = x + tileSize/2;
    int cy = y + tileSize/2;
    DrawCircle(cx, cy, tileSize/3 + 1, DARKBLUE);
    DrawCircle(cx, cy, tileSize/3, BLUE);
    DrawCircle(cx - 5, cy, tileSize/5, RAYWHITE);
    DrawCircle(cx + 5, cy, tileSize/5, RAYWHITE);
    DrawRectangle(cx - 5, cy - tileSize/5, 10, (tileSize/5)*2, RAYWHITE);
    DrawCircle(cx, cy, tileSize/8, DARKBLUE);
    DrawCircle(cx, cy, tileSize/16, BLACK);
}

// Draws the temporary enemy-kill powerup icon.
void DrawKillPowerupIcon(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    int cx = x + tileSize/2;
    int cy = y + tileSize/2;

    // Sword icon for temporary enemy-kill power.
    DrawLine(cx - 9, cy + 9, cx + 9, cy - 9, DARKGRAY);
    DrawLine(cx - 8, cy + 8, cx + 8, cy - 8, RAYWHITE);
    DrawLine(cx - 7, cy + 9, cx + 9, cy - 7, LIGHTGRAY);
    DrawCircle(cx + 9, cy - 9, 3, SKYBLUE);
    DrawRectangle(cx - 12, cy + 8, 12, 4, BROWN);
    DrawRectangle(cx - 6, cy + 3, 12, 4, GOLD);
}


// Draws the full-maze vision powerup icon.
void DrawRevealPowerupIcon(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    int cx = x + tileSize/2;
    int cy = y + tileSize/2;

    // Full-vision powerup: glowing lantern badge.
    DrawCircle(cx, cy, tileSize/3 + 2, DARKPURPLE);
    DrawCircle(cx, cy, tileSize/3, VIOLET);
    DrawCircle(cx, cy, tileSize/5, YELLOW);
    DrawCircle(cx, cy, tileSize/8, GOLD);
    DrawRectangle(cx - 5, cy - 12, 10, 4, DARKBROWN);
    DrawRectangle(cx - 4, cy + 8, 8, 5, DARKBROWN);
    DrawLine(cx - 10, cy, cx - 14, cy, GOLD);
    DrawLine(cx + 10, cy, cx + 14, cy, GOLD);
    DrawLine(cx, cy - 10, cx, cy - 14, GOLD);
}

// Draws the exit door tile.
void DrawExitIcon(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    DrawRectangle(x + 7, y + 5, tileSize - 14, tileSize - 8, PURPLE);
    DrawRectangle(x + 11, y + 9, tileSize - 22, tileSize - 12, PURPLE);
    DrawCircle(x + tileSize - 11, y + tileSize/2, 2, GOLD);
    DrawTriangle((Vector2){x + tileSize/2 - 5, y + tileSize - 8}, (Vector2){x + tileSize/2 + 5, y + tileSize - 8}, (Vector2){x + tileSize/2, y + tileSize - 2}, PURPLE);
}

// Draws an enemy character with raylib shapes.
void DrawEnemyCharacter(int tileX, int tileY, int tileSize) {
    int x = tileX * tileSize;
    int y = tileY * tileSize;
    DrawCircle(x + tileSize/2, y + tileSize/2, tileSize/3, RED);
    DrawTriangle((Vector2){x + 7, y + 7}, (Vector2){x + 12, y + 15}, (Vector2){x + 3, y + 15}, MAROON);
    DrawTriangle((Vector2){x + tileSize - 7, y + 7}, (Vector2){x + tileSize - 12, y + 15}, (Vector2){x + tileSize - 3, y + 15}, MAROON);
    DrawCircle(x + tileSize/2 - 5, y + tileSize/2 - 3, 3, WHITE);
    DrawCircle(x + tileSize/2 + 5, y + tileSize/2 - 3, 3, WHITE);
    DrawCircle(x + tileSize/2 - 5, y + tileSize/2 - 3, 1, BLACK);
    DrawCircle(x + tileSize/2 + 5, y + tileSize/2 - 3, 1, BLACK);
    DrawRectangle(x + tileSize/2 - 6, y + tileSize/2 + 7, 12, 3, MAROON);
}

// Handles typed text and backspace for the selected input box.
void UpdateTextInput(char *text, int maxLength) {
    int key = GetCharPressed();

    while (key > 0) {
        // Only allow normal visible characters except spaces, because file reading uses spaces as separators
        if (key >= 33 && key <= 126 && (int)strlen(text) < maxLength - 1) {
            int length = strlen(text);
            text[length] = (char)key;
            text[length + 1] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int length = strlen(text);
        if (length > 0) text[length - 1] = '\0';
    }
}

// Draws small background decorations
void DrawScreenDecoration(GameScreen screen) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    // Clean corner decorations 
    DrawCircle(48, 48, 5, SKYBLUE);
    DrawCircle(78, 38, 4, GOLD);
    DrawCircle(w - 48, 48, 5, SKYBLUE);
    DrawCircle(w - 78, 38, 4, GOLD);
    DrawLine(42, h - 42, 128, h - 42, GREEN);
    DrawLine(w - 128, h - 42, w - 42, h - 42, GREEN);

    int leftX = 72;
    int rightX = w - 72;
    int iconY = 120;

    if (screen == SCREEN_LOGIN) {
        // Account key icons
        DrawCircle(leftX, iconY, 12, SKYBLUE);
        DrawCircle(leftX, iconY - 12, 7, BEIGE);
        DrawCircle(rightX - 18, iconY - 2, 8, GOLD);
        DrawRectangle(rightX - 12, iconY - 6, 30, 6, GOLD);
        DrawRectangle(rightX + 8, iconY, 5, 10, GOLD);
    } else if (screen == SCREEN_MENU) {
        // Door and torch side icons
        DrawRectangle(leftX - 14, iconY - 18, 28, 42, PURPLE);
        DrawCircle(leftX + 8, iconY + 3, 3, GOLD);
        DrawRectangle(rightX - 4, iconY, 8, 18, BROWN);
        DrawCircle(rightX, iconY - 8, 9, ORANGE);
        DrawCircle(rightX, iconY - 11, 5, YELLOW);
    } else if (screen == SCREEN_TUTORIAL) {
        // Small book icons
        DrawRectangle(leftX - 22, iconY - 15, 22, 32, DARKBLUE);
        DrawRectangle(leftX, iconY - 15, 22, 32, BLUE);
        DrawLine(leftX, iconY - 15, leftX, iconY + 17, RAYWHITE);
        DrawRectangle(rightX - 22, iconY - 15, 22, 32, DARKBLUE);
        DrawRectangle(rightX, iconY - 15, 22, 32, BLUE);
        DrawLine(rightX, iconY - 15, rightX, iconY + 17, RAYWHITE);
    } else if (screen == SCREEN_LEADERBOARD) {
        // Trophy side icons
        DrawRectangle(leftX - 14, iconY - 15, 28, 20, GOLD);
        DrawCircle(leftX - 20, iconY - 8, 7, GOLD);
        DrawCircle(leftX + 20, iconY - 8, 7, GOLD);
        DrawRectangle(leftX - 5, iconY + 5, 10, 18, GOLD);
        DrawRectangle(leftX - 18, iconY + 23, 36, 5, GOLD);
        DrawRectangle(rightX - 14, iconY - 15, 28, 20, GOLD);
        DrawCircle(rightX - 20, iconY - 8, 7, GOLD);
        DrawCircle(rightX + 20, iconY - 8, 7, GOLD);
        DrawRectangle(rightX - 5, iconY + 5, 10, 18, GOLD);
        DrawRectangle(rightX - 18, iconY + 23, 36, 5, GOLD);
    } else if (screen == SCREEN_RESTART_CHOICE) {
        // Restart icons
        DrawLine(leftX - 22, iconY + 10, leftX + 6, iconY - 14, SKYBLUE);
        DrawLine(leftX + 6, iconY - 14, leftX + 27, iconY + 7, SKYBLUE);
        DrawTriangle((Vector2){leftX + 27, iconY + 7}, (Vector2){leftX + 14, iconY + 3}, (Vector2){leftX + 22, iconY - 7}, SKYBLUE);
        DrawLine(rightX - 22, iconY + 10, rightX + 6, iconY - 14, SKYBLUE);
        DrawLine(rightX + 6, iconY - 14, rightX + 27, iconY + 7, SKYBLUE);
        DrawTriangle((Vector2){rightX + 27, iconY + 7}, (Vector2){rightX + 14, iconY + 3}, (Vector2){rightX + 22, iconY - 7}, SKYBLUE);
    } else if (screen == SCREEN_GAMEOVER) {
        // Small skull icons
        DrawCircle(leftX, iconY, 21, MAROON);
        DrawCircle(leftX - 8, iconY - 4, 4, BLACK);
        DrawCircle(leftX + 8, iconY - 4, 4, BLACK);
        DrawRectangle(leftX - 10, iconY + 11, 20, 7, DARKGRAY);
        DrawCircle(rightX, iconY, 21, MAROON);
        DrawCircle(rightX - 8, iconY - 4, 4, BLACK);
        DrawCircle(rightX + 8, iconY - 4, 4, BLACK);
        DrawRectangle(rightX - 10, iconY + 11, 20, 7, DARKGRAY);
    }
}

// ==========================================
// 3. Dungeon, Enemy, and Game Setup Logic
// ==========================================

// Builds a random dungeon floor and places the exit and powerups.
void GenerateDungeon(MapData *map, int currentFloor) {
    // Begin with a solid wall grid. The random walk will carve floor tiles from it.
    for (int x = 0; x < MAX_X; x++) {
        for (int y = 0; y < MAX_Y; y++) {
            map->grid[x][y] = TILE_WALL;
        }
    }

    // Start carving from the center so the player spawn is always connected.
    int currentX = MAX_X / 2;
    int currentY = MAX_Y / 2;
    map->grid[currentX][currentY] = TILE_FLOOR;

    // Higher floors become more maze-like: fewer open floor tiles, more walls, and easier to get lost.
    // The scaling is gentle so the random maze stays playable.
    int floorsToDig = 350 - currentFloor * 3;
    if (floorsToDig < 250) floorsToDig = 250;

    // Random walk carving creates a different playable layout each floor.
    while (floorsToDig > 0) {
        int dir = GetRandomValue(0, 3);

        if (dir == 0 && currentY > 1) currentY--;
        else if (dir == 1 && currentY < MAX_Y - 2) currentY++;
        else if (dir == 2 && currentX > 1) currentX--;
        else if (dir == 3 && currentX < MAX_X - 2) currentX++;

        if (map->grid[currentX][currentY] == TILE_WALL) {
            map->grid[currentX][currentY] = TILE_FLOOR;
            floorsToDig--;
        }
    }

    // The exit is placed at the final carved position.
    map->grid[currentX][currentY] = TILE_EXIT;

    // Fewer powerups on early floors, but all powerups slowly increase as floor rises.
    int healthAmount = 1 + currentFloor / 7;
    int oilAmount = 1 + currentFloor / 8;
    int freezeAmount = (currentFloor >= 2) ? 1 + currentFloor / 10 : 0;
    int invisibleAmount = (currentFloor >= 3) ? 1 + currentFloor / 10 : 0;
    int killAmount = (currentFloor >= 4) ? 1 + currentFloor / 12 : 0;
    int revealAmount = (currentFloor >= 5) ? 1 + currentFloor / 14 : 0;

    if (healthAmount > 4) healthAmount = 4;
    if (oilAmount > 3) oilAmount = 3;
    if (freezeAmount > 2) freezeAmount = 2;
    if (invisibleAmount > 2) invisibleAmount = 2;
    if (killAmount > 2) killAmount = 2;
    if (revealAmount > 2) revealAmount = 2;

    int healthPlaced = 0;
    int oilPlaced = 0;
    int freezePlaced = 0;
    int invisiblePlaced = 0;
    int killPlaced = 0;
    int revealPlaced = 0;

    while (healthPlaced < healthAmount) {
        int x = GetRandomValue(1, MAX_X - 2);
        int y = GetRandomValue(1, MAX_Y - 2);
        if (map->grid[x][y] == TILE_FLOOR && (x != MAX_X/2 || y != MAX_Y/2)) {
            map->grid[x][y] = TILE_HEALTH_PICKUP;
            healthPlaced++;
        }
    }

    while (oilPlaced < oilAmount) {
        int x = GetRandomValue(1, MAX_X - 2);
        int y = GetRandomValue(1, MAX_Y - 2);
        if (map->grid[x][y] == TILE_FLOOR && (x != MAX_X/2 || y != MAX_Y/2)) {
            map->grid[x][y] = TILE_OIL_PICKUP;
            oilPlaced++;
        }
    }

    while (freezePlaced < freezeAmount) {
        int x = GetRandomValue(1, MAX_X - 2);
        int y = GetRandomValue(1, MAX_Y - 2);
        if (map->grid[x][y] == TILE_FLOOR && (x != MAX_X/2 || y != MAX_Y/2)) {
            map->grid[x][y] = TILE_FREEZE_PICKUP;
            freezePlaced++;
        }
    }

    while (invisiblePlaced < invisibleAmount) {
        int x = GetRandomValue(1, MAX_X - 2);
        int y = GetRandomValue(1, MAX_Y - 2);
        if (map->grid[x][y] == TILE_FLOOR && (x != MAX_X/2 || y != MAX_Y/2)) {
            map->grid[x][y] = TILE_INVISIBLE_PICKUP;
            invisiblePlaced++;
        }
    }

    while (killPlaced < killAmount) {
        int x = GetRandomValue(1, MAX_X - 2);
        int y = GetRandomValue(1, MAX_Y - 2);
        if (map->grid[x][y] == TILE_FLOOR && (x != MAX_X/2 || y != MAX_Y/2)) {
            map->grid[x][y] = TILE_KILL_PICKUP;
            killPlaced++;
        }
    }

    while (revealPlaced < revealAmount) {
        int x = GetRandomValue(1, MAX_X - 2);
        int y = GetRandomValue(1, MAX_Y - 2);
        if (map->grid[x][y] == TILE_FLOOR && (x != MAX_X/2 || y != MAX_Y/2)) {
            map->grid[x][y] = TILE_REVEAL_PICKUP;
            revealPlaced++;
        }
    }

    // Keep the starting tile clear for the player.
    map->grid[MAX_X / 2][MAX_Y / 2] = TILE_FLOOR;
}

// Places enemies on safe floor tiles away from the player spawn point.
void SpawnEnemies(Entity enemies[], MapData *map, int currentFloor) {
    int enemyCount = 2 + currentFloor + currentFloor / 5;
    if (enemyCount > MAX_ENEMIES) enemyCount = MAX_ENEMIES;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (i < enemyCount) {
            enemies[i].health = 10;
            int ex, ey;
            do {
                ex = GetRandomValue(1, MAX_X - 2);
                ey = GetRandomValue(1, MAX_Y - 2);
            } while (map->grid[ex][ey] != TILE_FLOOR || (abs(ex - MAX_X/2) < 3 && abs(ey - MAX_Y/2) < 3));
            enemies[i].position = (Vector2){ ex, ey };
        } else {
            enemies[i].health = 0;
        }
    }
}

// Starts a new run from Floor 1.
void InitGame(Entity *player, Entity enemies[], MapData *map, int *floor, float *timer, GameMode mode, float *freezeTimer, float *invisibleTimer, float *killTimer, float *revealTimer) {
    *floor = 1;
    GenerateDungeon(map, *floor);

    player->position = (Vector2){ MAX_X / 2, MAX_Y / 2 };
    player->health = 100;
    player->torchRadius = 2.5f;

    if (mode == MODE_TIMED) *timer = 60.0f;
    else *timer = 0.0f;

    *freezeTimer = 0.0f;
    *invisibleTimer = 0.0f;
    *killTimer = 0.0f;
    *revealTimer = 0.0f;

    SpawnEnemies(enemies, map, *floor);
}


// Starts a new run from Floor 1.
// Starts or restarts a run from a selected floor.
void InitGameAtFloor(Entity *player, Entity enemies[], MapData *map, int targetFloor, float *timer, GameMode mode, float *freezeTimer, float *invisibleTimer, float *killTimer, float *revealTimer) {
    if (targetFloor < 1) targetFloor = 1;
    GenerateDungeon(map, targetFloor);

    player->position = (Vector2){ MAX_X / 2, MAX_Y / 2 };
    player->health = 100;
    player->torchRadius = 2.5f;

    if (mode == MODE_TIMED) *timer = 60.0f + (targetFloor - 1) * 15.0f;
    else *timer = 0.0f;

    *freezeTimer = 0.0f;
    *invisibleTimer = 0.0f;
    *killTimer = 0.0f;
    *revealTimer = 0.0f;

    SpawnEnemies(enemies, map, targetFloor);
}

// ==========================================
// 4. Main Game Loop
// ==========================================

// Program entry point. Initializes raylib, runs the game loop, and closes resources.
int main(void) {
    // Create the game window and set the frame rate.
    InitWindow(960, 760, "Endless Depths");
    SetTargetFPS(60);
    SetExitKey(0);

    // Load one looping background music file for the whole game.
    InitAudioDevice();
    bool musicEnabled = true;
    bool bgmLoaded = FileExistsSimple("resources/music/bgm.mp3");
    Music bgm = { 0 };
    if (bgmLoaded) {
        bgm = LoadMusicStream("resources/music/bgm.mp3");
        bgm.looping = true;
        PlayMusicStream(bgm);
    }

    // Main state variables control the active screen, mode, player, map, and timers.
    GameScreen currentScreen = SCREEN_LOGIN;
    GameScreen restartCancelScreen = SCREEN_GAMEPLAY;
    GameMode currentMode = MODE_BASIC;

    Entity player;
    Entity enemies[MAX_ENEMIES];
    MapData map;

    int currentFloor = 1;
    float timeLeft = 0.0f;
    float enemyMoveTimer = 0.0f;
    float freezeTimer = 0.0f;
    float invisibleTimer = 0.0f;
    float killTimer = 0.0f;
    float revealTimer = 0.0f;
    float playerMoveTimer = 0.0f;
    float playerMoveHoldTimer = 0.0f;
    bool exitWindow = false;

    // Highest floors are loaded per account after login.
    int highScoreBasic = 0;
    int highScoreTimed = 0;

    // Login page text buffers and account state.
    char currentUsername[MAX_NAME_LENGTH] = "Guest";
    char usernameInput[MAX_NAME_LENGTH] = "";
    char passwordInput[MAX_PASSWORD_LENGTH] = "";
    char loginMessage[128] = "";
    bool creatingAccount = false;
    int activeInput = -1; // -1 = no box selected, 0 = username, 1 = password
    GameMode leaderboardMode = MODE_BASIC;

    // Main loop: handle input, update game state, then draw the current screen.
    while (!exitWindow && !WindowShouldClose()) {
        float dt = GetFrameTime();

        // Keep music streaming and handle music toggles.
        if (bgmLoaded) {
            if (musicEnabled) UpdateMusicStream(bgm);

            // Gameplay uses keyboard only. Login/Menu/Game Over use the bottom-center mouse button.
            bool toggleMusic = false;
            if (currentScreen == SCREEN_GAMEPLAY) {
                if (IsKeyPressed(KEY_P)) toggleMusic = true;
            } else if (ShouldShowMusicMouseButton(currentScreen)) {
                if (IsButtonClicked(GetMusicButton())) toggleMusic = true;
            }

            if (toggleMusic) {
                musicEnabled = !musicEnabled;
                if (musicEnabled) ResumeMusicStream(bgm);
                else PauseMusicStream(bgm);
            }
        }

        // ------------------------------------------
        // [SECTION A]: Update Logic
        // ------------------------------------------
        switch(currentScreen) {
            case SCREEN_LOGIN: {
                // Account screen: choose login/create, type fields, confirm, or quit.
                // Login/Create and input fields are controlled by mouse clicks only.
                // This avoids keyboard conflicts with usernames/passwords such as abc123.
                int formW = 560;
                int formX = GetScreenWidth()/2 - formW/2;
                int formY = 250;
                int modeY = formY + 60;
                int boxX = formX + 220;
                int boxW = 300;
                Rectangle loginButton = { formX + 70, modeY, 170, 44 };
                Rectangle createButton = { formX + 320, modeY, 170, 44 };
                Rectangle usernameBox = { boxX, formY + 145, boxW, 50 };
                Rectangle passwordBox = { boxX, formY + 215, boxW, 50 };
                Rectangle confirmButton = { formX + 100, formY + 350, 160, 48 };
                Rectangle quitButton = { formX + 300, formY + 350, 160, 48 };

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mouse = GetMousePosition();
                    if (CheckCollisionPointRec(mouse, loginButton)) {
                        creatingAccount = false;
                    } else if (CheckCollisionPointRec(mouse, createButton)) {
                        creatingAccount = true;
                    } else if (CheckCollisionPointRec(mouse, usernameBox)) {
                        activeInput = 0;
                    } else if (CheckCollisionPointRec(mouse, passwordBox)) {
                        activeInput = 1;
                    } else if (CheckCollisionPointRec(mouse, confirmButton)) {
                        if (strlen(usernameInput) == 0 || strlen(passwordInput) == 0) {
                            strcpy(loginMessage, "Please enter both username and password.");
                        } else if (creatingAccount) {
                            if (AccountExists(usernameInput)) {
                                strcpy(loginMessage, "Username already exists. Choose another username.");
                            } else if (CreateAccount(usernameInput, passwordInput)) {
                                strcpy(currentUsername, usernameInput);
                                EnsureContinueFloorsForUser(currentUsername);
                                LoadPlayerHighScores(currentUsername, &highScoreBasic, &highScoreTimed);
                                if (highScoreBasic > 0) UpdatePlayerBestFloor(currentUsername, MODE_BASIC, highScoreBasic, &highScoreBasic, &highScoreTimed);
                                if (highScoreTimed > 0) UpdatePlayerBestFloor(currentUsername, MODE_TIMED, highScoreTimed, &highScoreBasic, &highScoreTimed);
                                currentScreen = SCREEN_MENU;
                                strcpy(loginMessage, "Account created successfully.");
                            } else {
                                strcpy(loginMessage, "Account creation failed. Try again.");
                            }
                        } else {
                            if (ValidateLogin(usernameInput, passwordInput)) {
                                strcpy(currentUsername, usernameInput);
                                EnsureContinueFloorsForUser(currentUsername);
                                LoadPlayerHighScores(currentUsername, &highScoreBasic, &highScoreTimed);
                                if (highScoreBasic > 0) UpdatePlayerBestFloor(currentUsername, MODE_BASIC, highScoreBasic, &highScoreBasic, &highScoreTimed);
                                if (highScoreTimed > 0) UpdatePlayerBestFloor(currentUsername, MODE_TIMED, highScoreTimed, &highScoreBasic, &highScoreTimed);
                                currentScreen = SCREEN_MENU;
                                strcpy(loginMessage, "Login successful.");
                            } else {
                                strcpy(loginMessage, "Login failed. Check username/password or create account.");
                            }
                        }
                    } else if (CheckCollisionPointRec(mouse, quitButton)) {
                        exitWindow = true;
                    } else {
                        activeInput = -1;
                    }
                }

                if (activeInput == 0) UpdateTextInput(usernameInput, MAX_NAME_LENGTH);
                else if (activeInput == 1) UpdateTextInput(passwordInput, MAX_PASSWORD_LENGTH);
            } break;

            case SCREEN_MENU: {
                // Menu screen: start, continue, change mode, open pages, logout, or quit.
                int cx = GetScreenWidth()/2;
                int lastFloor = LoadLastFloor(currentUsername, currentMode);
                Rectangle startButton = { cx - 145, 300, 290, 46 };
                Rectangle continueButton = { cx - 145, 356, 290, 46 };
                Rectangle modeButton = { cx - 145, 412, 290, 42 };
                Rectangle tutorialButton = { cx - 145, 464, 290, 42 };
                Rectangle leaderboardButton = { cx - 145, 516, 290, 42 };
                Rectangle logoutButton = { cx - 145, 568, 290, 42 };
                Rectangle quitButton = { cx - 145, 620, 290, 42 };

                if (IsButtonClicked(startButton)) {
                    InitGame(&player, enemies, &map, &currentFloor, &timeLeft, currentMode, &freezeTimer, &invisibleTimer, &killTimer, &revealTimer);
                    SaveLastFloor(currentUsername, currentMode, currentFloor);
                    enemyMoveTimer = 0.0f;
                    playerMoveTimer = 0.0f;
                    playerMoveHoldTimer = 0.0f;
                    currentScreen = SCREEN_GAMEPLAY;
                }
                if (IsButtonClicked(continueButton)) {
                    currentFloor = lastFloor > 0 ? lastFloor : 1;
                    InitGameAtFloor(&player, enemies, &map, currentFloor, &timeLeft, currentMode, &freezeTimer, &invisibleTimer, &killTimer, &revealTimer);
                    if (currentFloor > 1) UpdatePlayerBestFloor(currentUsername, currentMode, currentFloor, &highScoreBasic, &highScoreTimed);
                    SaveLastFloor(currentUsername, currentMode, currentFloor);
                    enemyMoveTimer = 0.0f;
                    playerMoveTimer = 0.0f;
                    playerMoveHoldTimer = 0.0f;
                    currentScreen = SCREEN_GAMEPLAY;
                }
                if (IsButtonClicked(modeButton)) currentMode = (currentMode == MODE_BASIC) ? MODE_TIMED : MODE_BASIC;
                if (IsButtonClicked(tutorialButton)) currentScreen = SCREEN_TUTORIAL;
                if (IsButtonClicked(leaderboardButton)) {
                    leaderboardMode = currentMode;
                    currentScreen = SCREEN_LEADERBOARD;
                }
                if (IsButtonClicked(logoutButton)) {
                    strcpy(currentUsername, "Guest");
                    usernameInput[0] = '\0';
                    passwordInput[0] = '\0';
                    creatingAccount = false;
                    activeInput = -1;
                    strcpy(loginMessage, "");
                    highScoreBasic = 0;
                    highScoreTimed = 0;
                    currentScreen = SCREEN_LOGIN;
                }
                if (IsButtonClicked(quitButton)) exitWindow = true;
            } break;

            case SCREEN_TUTORIAL: {
                // Tutorial screen: return to the main menu.
                Rectangle backButton = { GetScreenWidth()/2 - 120, 680, 240, 46 };
                if (IsButtonClicked(backButton)) currentScreen = SCREEN_MENU;
            } break;

            case SCREEN_LEADERBOARD: {
                // Leaderboard screen: switch mode table or return to menu.
                int cx = GetScreenWidth()/2;
                Rectangle basicButton = { cx - 250, 180, 230, 42 };
                Rectangle timedButton = { cx + 20, 180, 230, 42 };
                Rectangle backButton = { cx - 120, 655, 240, 46 };
                if (IsButtonClicked(basicButton)) leaderboardMode = MODE_BASIC;
                if (IsButtonClicked(timedButton)) leaderboardMode = MODE_TIMED;
                if (IsButtonClicked(backButton)) currentScreen = SCREEN_MENU;
            } break;

            case SCREEN_GAMEPLAY: {
                // Gameplay screen: update timers, movement, pickups, enemies, and floor changes.
                // Gameplay uses keyboard shortcuts for quick menu and restart access.
                if (IsKeyPressed(KEY_M)) {
                    SaveLastFloor(currentUsername, currentMode, currentFloor);
                    currentScreen = SCREEN_MENU;
                }
                if (IsKeyPressed(KEY_R)) {
                    restartCancelScreen = SCREEN_GAMEPLAY;
                    currentScreen = SCREEN_RESTART_CHOICE;
                }

                bool isDead = false;

                if (freezeTimer > 0) freezeTimer -= dt;
                if (invisibleTimer > 0) invisibleTimer -= dt;
                if (killTimer > 0) killTimer -= dt;
                if (revealTimer > 0) revealTimer -= dt;
                if (freezeTimer < 0) freezeTimer = 0;
                if (invisibleTimer < 0) invisibleTimer = 0;
                if (killTimer < 0) killTimer = 0;
                if (revealTimer < 0) revealTimer = 0;

                // In timed mode, freeze powerup pauses the countdown.
                if (currentMode == MODE_TIMED && freezeTimer <= 0) {
                    timeLeft -= dt;
                    if (timeLeft <= 0) isDead = true;
                }

                if (player.health <= 0) isDead = true;

                if (isDead) {
                    UpdatePlayerBestFloor(currentUsername, currentMode, currentFloor, &highScoreBasic, &highScoreTimed);
                    SaveLastFloor(currentUsername, currentMode, currentFloor);
                    currentScreen = SCREEN_GAMEOVER;
                }

                if (!isDead) {
                    if (freezeTimer <= 0) enemyMoveTimer += dt;

                    int nextX = (int)player.position.x;
                    int nextY = (int)player.position.y;
                    bool playerMoved = false;

                    // WASD + Arrow keys movement:
                    // - Press once = move exactly one tile.
                    // - Hold the key = after a short delay, keep moving tile-by-tile until released.
                    playerMoveTimer -= dt;
                    bool holdingMoveKey = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN) ||
                                          IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
                    bool pressedMoveKey = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN) ||
                                          IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);

                    if (!holdingMoveKey) {
                        playerMoveTimer = 0.0f;
                        playerMoveHoldTimer = 0.0f;
                    }
                    else {
                        playerMoveHoldTimer += dt;

                        if (pressedMoveKey) {
                            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) { nextY -= 1; playerMoved = true; }
                            else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) { nextY += 1; playerMoved = true; }
                            else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) { nextX -= 1; playerMoved = true; }
                            else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) { nextX += 1; playerMoved = true; }

                            playerMoveTimer = 0.12f;
                            playerMoveHoldTimer = 0.0f;
                        }
                        else if (playerMoveHoldTimer >= 0.28f && playerMoveTimer <= 0.0f) {
                            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { nextY -= 1; playerMoved = true; }
                            else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { nextY += 1; playerMoved = true; }
                            else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) { nextX -= 1; playerMoved = true; }
                            else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { nextX += 1; playerMoved = true; }

                            playerMoveTimer = 0.12f;
                        }
                    }

                    if (playerMoved && IsInsideMap(nextX, nextY)) {
                        bool hitEnemy = false;

                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            if (enemies[i].health > 0 && nextX == (int)enemies[i].position.x && nextY == (int)enemies[i].position.y) {
                                hitEnemy = true;
                                if (killTimer > 0) {
                                    enemies[i].health = 0;
                                    hitEnemy = false;
                                } else if (invisibleTimer <= 0) {
                                    player.health -= 15;
                                }
                                break;
                            }
                        }

                        // If invisible, the player can safely pass through enemies.
                        if ((!hitEnemy || invisibleTimer > 0) && map.grid[nextX][nextY] != TILE_WALL) {
                            player.position.x = nextX;
                            player.position.y = nextY;

                            if (map.grid[nextX][nextY] == TILE_HEALTH_PICKUP) {
                                player.health += 20;
                                if (player.health > 100) player.health = 100;
                                map.grid[nextX][nextY] = TILE_FLOOR;
                            }
                            else if (map.grid[nextX][nextY] == TILE_OIL_PICKUP) {
                                player.torchRadius += 0.5f;
                                map.grid[nextX][nextY] = TILE_FLOOR;
                            }
                            else if (map.grid[nextX][nextY] == TILE_FREEZE_PICKUP) {
                                freezeTimer = 5.0f;
                                map.grid[nextX][nextY] = TILE_FLOOR;
                            }
                            else if (map.grid[nextX][nextY] == TILE_INVISIBLE_PICKUP) {
                                invisibleTimer = 5.0f;
                                map.grid[nextX][nextY] = TILE_FLOOR;
                            }
                            else if (map.grid[nextX][nextY] == TILE_KILL_PICKUP) {
                                killTimer = 5.0f;
                                map.grid[nextX][nextY] = TILE_FLOOR;
                            }
                            else if (map.grid[nextX][nextY] == TILE_REVEAL_PICKUP) {
                                revealTimer = 2.0f;
                                map.grid[nextX][nextY] = TILE_FLOOR;
                            }
                            else if (map.grid[nextX][nextY] == TILE_EXIT) {
                                currentFloor++;
                                UpdatePlayerBestFloor(currentUsername, currentMode, currentFloor, &highScoreBasic, &highScoreTimed);
                                SaveLastFloor(currentUsername, currentMode, currentFloor);
                                GenerateDungeon(&map, currentFloor);
                                player.position.x = MAX_X / 2;
                                player.position.y = MAX_Y / 2;
                                player.torchRadius = 2.5f;
                                freezeTimer = 0.0f;
                                invisibleTimer = 0.0f;
                                killTimer = 0.0f;
                                revealTimer = 0.0f;

                                if (currentMode == MODE_TIMED) {
                                    timeLeft = 60.0f + (currentFloor - 1) * 15.0f;
                                }

                                SpawnEnemies(enemies, &map, currentFloor);
                            }
                        }
                    }

                    float enemySpeed = 0.64f - (currentFloor * 0.028f);
                    if (enemySpeed < 0.19f) enemySpeed = 0.19f;

                    // Enemies stop moving while freeze is active.
                    if (freezeTimer <= 0 && enemyMoveTimer >= enemySpeed) {
                        enemyMoveTimer = 0.0f;

                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            if (enemies[i].health > 0) {
                                int nextEX = (int)enemies[i].position.x;
                                int nextEY = (int)enemies[i].position.y;

                                int dir = GetRandomValue(0, 3);
                                if (dir == 0) nextEY -= 1;
                                else if (dir == 1) nextEY += 1;
                                else if (dir == 2) nextEX -= 1;
                                else if (dir == 3) nextEX += 1;

                                if (!IsInsideMap(nextEX, nextEY)) continue;

                                if (nextEX == (int)player.position.x && nextEY == (int)player.position.y) {
                                    if (killTimer > 0) enemies[i].health = 0;
                                    else if (invisibleTimer <= 0) player.health -= 15;
                                }
                                else if (map.grid[nextEX][nextEY] != TILE_WALL && map.grid[nextEX][nextEY] != TILE_EXIT) {
                                    enemies[i].position.x = nextEX;
                                    enemies[i].position.y = nextEY;
                                }
                            }
                        }
                    }
                }
            } break;

            case SCREEN_RESTART_CHOICE: {
                // Restart screen: choose the restart type, cancel, or return to menu.
                int cx = GetScreenWidth()/2;
                Rectangle floorOneButton = { cx - 205, 355, 410, 54 };
                Rectangle currentFloorButton = { cx - 205, 430, 410, 54 };
                Rectangle cancelButton = { cx - 205, 515, 195, 48 };
                Rectangle menuButton = { cx + 10, 515, 195, 48 };

                if (IsButtonClicked(floorOneButton)) {
                    InitGame(&player, enemies, &map, &currentFloor, &timeLeft, currentMode, &freezeTimer, &invisibleTimer, &killTimer, &revealTimer);
                    enemyMoveTimer = 0.0f;
                    playerMoveTimer = 0.0f;
                    playerMoveHoldTimer = 0.0f;
                    currentScreen = SCREEN_GAMEPLAY;
                }
                if (IsButtonClicked(currentFloorButton)) {
                    InitGameAtFloor(&player, enemies, &map, currentFloor, &timeLeft, currentMode, &freezeTimer, &invisibleTimer, &killTimer, &revealTimer);
                    enemyMoveTimer = 0.0f;
                    playerMoveTimer = 0.0f;
                    playerMoveHoldTimer = 0.0f;
                    currentScreen = SCREEN_GAMEPLAY;
                }
                if (IsButtonClicked(cancelButton)) currentScreen = restartCancelScreen;
                if (IsButtonClicked(menuButton)) currentScreen = SCREEN_MENU;
            } break;

            case SCREEN_GAMEOVER: {
                // Game over screen: restart, return to menu, view leaderboard, logout, or quit.
                int cx = GetScreenWidth()/2;
                Rectangle restartButton = { cx - 145, 420, 290, 44 };
                Rectangle menuButton = { cx - 145, 475, 290, 44 };
                Rectangle leaderboardButton = { cx - 145, 530, 290, 44 };
                Rectangle logoutButton = { cx - 145, 585, 290, 44 };
                Rectangle quitButton = { cx - 145, 640, 290, 40 };

                if (IsButtonClicked(restartButton)) {
                    restartCancelScreen = SCREEN_GAMEOVER;
                    currentScreen = SCREEN_RESTART_CHOICE;
                }
                if (IsButtonClicked(menuButton)) currentScreen = SCREEN_MENU;
                if (IsButtonClicked(leaderboardButton)) {
                    leaderboardMode = currentMode;
                    currentScreen = SCREEN_LEADERBOARD;
                }
                if (IsButtonClicked(logoutButton)) {
                    strcpy(currentUsername, "Guest");
                    usernameInput[0] = '\0';
                    passwordInput[0] = '\0';
                    creatingAccount = false;
                    activeInput = -1;
                    strcpy(loginMessage, "");
                    highScoreBasic = 0;
                    highScoreTimed = 0;
                    currentScreen = SCREEN_LOGIN;
                }
                if (IsButtonClicked(quitButton)) exitWindow = true;
            } break;
        }

        // ------------------------------------------
        // [SECTION B]: Draw Logic
        // ------------------------------------------
        BeginDrawing();
        ClearBackground(BLACK);
        if (currentScreen != SCREEN_GAMEPLAY) DrawScreenDecoration(currentScreen);

        switch(currentScreen) {
            case SCREEN_LOGIN: {
                // Draw the account form, input boxes, and confirm/quit buttons.
                int formW = 560;
                int formX = GetScreenWidth()/2 - formW/2;
                int formY = 250;
                int modeY = formY + 60;
                int labelX = formX + 40;
                int boxX = formX + 220;
                int boxW = 300;

                DrawTextCentered("ENDLESS DEPTHS", 62, 66, RAYWHITE);
                DrawTextCentered(creatingAccount ? "CREATE ACCOUNT" : "LOGIN ACCOUNT", 185, 40, YELLOW);

                DrawRectangleLines(formX, formY, formW, 330, GRAY);
                DrawRectangleLines(formX + 70, modeY, 170, 44, !creatingAccount ? GOLD : GRAY);
                DrawRectangleLines(formX + 320, modeY, 170, 44, creatingAccount ? YELLOW : GRAY);
                DrawTextBoxCentered("LOGIN", formX + 70, modeY + 9, 170, 25, !creatingAccount ? GOLD : LIGHTGRAY);
                DrawTextBoxCentered("CREATE", formX + 320, modeY + 9, 170, 25, creatingAccount ? YELLOW : LIGHTGRAY);

                DrawTextClear("Username", labelX, formY + 155, 28, LIGHTGRAY);
                DrawRectangleLines(boxX, formY + 145, boxW, 50, activeInput == 0 ? YELLOW : GRAY);
                DrawTextClear(usernameInput, boxX + 12, formY + 158, 26, WHITE);

                DrawTextClear("Password", labelX, formY + 225, 28, LIGHTGRAY);
                DrawRectangleLines(boxX, formY + 215, boxW, 50, activeInput == 1 ? YELLOW : GRAY);

                char hiddenPassword[MAX_PASSWORD_LENGTH] = "";
                for (int i = 0; i < (int)strlen(passwordInput); i++) strcat(hiddenPassword, "*");
                DrawTextClear(hiddenPassword, boxX + 12, formY + 228, 26, WHITE);

                if (strlen(loginMessage) > 0) DrawTextCentered(loginMessage, formY + 285, 19, ORANGE);
                DrawUIButton((Rectangle){ formX + 100, formY + 350, 160, 48 }, "Confirm", 24, SKYBLUE, WHITE);
                DrawUIButton((Rectangle){ formX + 300, formY + 350, 160, 48 }, "Quit", 24, RED, WHITE);
            } break;

            case SCREEN_MENU:
                // Draw the main menu, current mode, high score, and navigation buttons.
                DrawTextCentered("ENDLESS DEPTHS", 65, 70, RAYWHITE);
                DrawTextCentered(TextFormat("Logged in as: %s", currentUsername), 145, 24, SKYBLUE);

                if (currentMode == MODE_BASIC) {
                    DrawTextCentered("< Mode: BASIC (No Time Limit) >", 205, 30, GREEN);
                    if (highScoreBasic > 0) DrawTextCentered(TextFormat("Highest Score: Floor %d", highScoreBasic), 245, 24, GOLD);
                    else DrawTextCentered("Highest Score: None", 245, 24, GOLD);
                } else {
                    DrawTextCentered("< Mode: TIMED (Timer Resets per Floor) >", 205, 30, RED);
                    if (highScoreTimed > 0) DrawTextCentered(TextFormat("Highest Score: Floor %d", highScoreTimed), 245, 24, GOLD);
                    else DrawTextCentered("Highest Score: None", 245, 24, GOLD);
                }

                {
                    int cx = GetScreenWidth()/2;
                    int lastFloor = LoadLastFloor(currentUsername, currentMode);
                    DrawUIButton((Rectangle){ cx - 145, 300, 290, 46 }, "Start New Game", 24, SKYBLUE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 356, 290, 46 }, TextFormat("Continue Floor %d", lastFloor > 0 ? lastFloor : 1), 23, SKYBLUE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 412, 290, 42 }, "Change Mode", 22, GOLD, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 464, 290, 42 }, "Tutorial", 22, SKYBLUE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 516, 290, 42 }, "Leaderboard", 22, SKYBLUE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 568, 290, 42 }, "Log Out / Switch", 22, ORANGE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 620, 290, 42 }, "Quit Game", 22, RED, WHITE);
                }
                break;

            case SCREEN_TUTORIAL:
                // Draw the rules and powerup explanations.
                DrawTextCentered("--- HOW TO PLAY ---", 72, 38, YELLOW);

                // Tutorial
                DrawTextCentered("1. MOVEMENT", 150, 26, SKYBLUE);
                DrawTextCentered("Hold W/A/S/D or ARROW KEYS to keep moving. Walls block paths.", 183, 22, WHITE);

                DrawTextCentered("2. GOAL", 230, 26, PURPLE);
                DrawTextCentered("Reach the PURPLE DOOR. Vision resets when you enter a new floor.", 263, 22, WHITE);

                DrawTextCentered("3. SURVIVE", 310, 26, RED);
                DrawTextCentered("Avoid enemies. Each hit costs 15 HP. HP carries between floors.", 343, 22, WHITE);

                DrawTextCentered("4. POWERUPS", 395, 26, GOLD);
                DrawTextCentered("GREEN PLUS: +20 HP, max 100.", 430, 21, GREEN);
                DrawTextCentered("TORCH: increases vision radius.", 458, 21, GOLD);
                DrawTextCentered("CYAN SNOWFLAKE: freezes enemies; Timed mode freezes time too.", 486, 21, SKYBLUE);
                DrawTextCentered("BLUE EYE: enemies cannot hurt you for a short time.", 514, 21, BLUE);
                DrawTextCentered("SWORD: temporarily kills enemies you touch.", 542, 21, RED);
                DrawTextCentered("LANTERN: reveals the whole maze for a short time.", 570, 21, VIOLET);

                DrawTextCentered("Higher floors become more maze-like, with tougher enemies and more powerups.", 620, 21, WHITE);
                DrawUIButton((Rectangle){ GetScreenWidth()/2 - 120, 680, 240, 46 }, "Back to Menu", 24, SKYBLUE, WHITE);
                break;

            case SCREEN_LEADERBOARD: {
                // Draw the selected mode leaderboard table.
                LeaderboardEntry entries[200];
                int count = LoadLeaderboard(entries, leaderboardMode);

                DrawTextCentered("--- LEADERBOARD ---", 55, 42, GOLD);
                DrawTextCentered(leaderboardMode == MODE_BASIC ? "[ BASIC MODE LEADERBOARD ]" : "[ TIMED MODE LEADERBOARD ]", 115, 28, leaderboardMode == MODE_BASIC ? GREEN : RED);
                {
                    int cx = GetScreenWidth()/2;
                    DrawUIButton((Rectangle){ cx - 250, 180, 230, 42 }, "Basic", 22, leaderboardMode == MODE_BASIC ? GOLD : GRAY, WHITE);
                    DrawUIButton((Rectangle){ cx + 20, 180, 230, 42 }, "Timed", 22, leaderboardMode == MODE_TIMED ? GOLD : GRAY, WHITE);
                }

                int tableW = 700;
                int tableX = GetScreenWidth()/2 - tableW/2;
                int tableY = 240;
                DrawRectangleLines(tableX, tableY, tableW, 390, GRAY);
                DrawTextClear("Rank", tableX + 45, tableY + 25, 25, LIGHTGRAY);
                DrawTextClear("Player", tableX + 190, tableY + 25, 25, LIGHTGRAY);
                DrawTextClear("Floor", tableX + 515, tableY + 25, 25, LIGHTGRAY);
                DrawLine(tableX + 20, tableY + 65, tableX + tableW - 20, tableY + 65, GRAY);

                if (count == 0) {
                    DrawTextCentered("No scores yet for this mode. Play a game first!", 410, 26, GRAY);
                } else {
                    for (int i = 0; i < count; i++) {
                        int y = tableY + 90 + i * 30;
                        DrawTextClear(TextFormat("%2d", i + 1), tableX + 50, y, 23, WHITE);
                        DrawTextClear(entries[i].username, tableX + 190, y, 23, WHITE);
                        DrawTextClear(TextFormat("Floor %d", entries[i].floor), tableX + 515, y, 23, GOLD);
                    }
                }

                DrawUIButton((Rectangle){ GetScreenWidth()/2 - 120, 655, 240, 46 }, "Back to Menu", 23, SKYBLUE, WHITE);
            } break;

            case SCREEN_GAMEPLAY:
                // Draw the visible dungeon, entities, HUD, and controls.
                for (int x = 0; x < MAX_X; x++) {
                    for (int y = 0; y < MAX_Y; y++) {
                        float dx = x - player.position.x;
                        float dy = y - player.position.y;
                        float distance = sqrt(dx*dx + dy*dy);

                        if (revealTimer > 0 || distance <= player.torchRadius) {
                            if (map.grid[x][y] == TILE_WALL) DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, WALL_COLOR);
                            else if (map.grid[x][y] == TILE_HEALTH_PICKUP) { DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR); DrawHealthPowerupIcon(x, y, TILE_SIZE); }
                            else if (map.grid[x][y] == TILE_OIL_PICKUP) { DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR); DrawOilPowerupIcon(x, y, TILE_SIZE); }
                            else if (map.grid[x][y] == TILE_FREEZE_PICKUP) { DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR); DrawFreezePowerupIcon(x, y, TILE_SIZE); }
                            else if (map.grid[x][y] == TILE_INVISIBLE_PICKUP) { DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR); DrawInvisiblePowerupIcon(x, y, TILE_SIZE); }
                            else if (map.grid[x][y] == TILE_KILL_PICKUP) { DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR); DrawKillPowerupIcon(x, y, TILE_SIZE); }
                            else if (map.grid[x][y] == TILE_REVEAL_PICKUP) { DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR); DrawRevealPowerupIcon(x, y, TILE_SIZE); }
                            else if (map.grid[x][y] == TILE_EXIT) { DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR); DrawExitIcon(x, y, TILE_SIZE); }
                            else DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, FLOOR_COLOR);
                        }
                    }
                }

                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].health > 0) {
                        float dx = enemies[i].position.x - player.position.x;
                        float dy = enemies[i].position.y - player.position.y;
                        if (revealTimer > 0 || sqrt(dx*dx + dy*dy) <= player.torchRadius) {
                            DrawEnemyCharacter((int)enemies[i].position.x, (int)enemies[i].position.y, TILE_SIZE);
                        }
                    }
                }

                DrawPlayerCharacter((int)player.position.x, (int)player.position.y, TILE_SIZE, invisibleTimer > 0);

                if (currentMode == MODE_TIMED) {
                    DrawTextCentered(TextFormat("Player: %s   Floor: %d   HP: %d/100   Time: %02.0fs", currentUsername, currentFloor, player.health, timeLeft), 8, 24, GREEN);
                } else {
                    DrawTextCentered(TextFormat("Player: %s   Floor: %d   HP: %d/100   Mode: Basic", currentUsername, currentFloor, player.health), 8, 24, GREEN);
                }

                int effectY = 620;
                if (freezeTimer > 0) { DrawTextClear(TextFormat("FREEZE: %.1fs", freezeTimer), 20, effectY, 24, SKYBLUE); effectY += 28; }
                if (invisibleTimer > 0) { DrawTextClear(TextFormat("INVISIBLE: %.1fs", invisibleTimer), 20, effectY, 24, BLUE); effectY += 28; }
                if (killTimer > 0) { DrawTextClear(TextFormat("SLASH: %.1fs", killTimer), 20, effectY, 24, RED); effectY += 28; }
                if (revealTimer > 0) { DrawTextClear(TextFormat("FULL VISION: %.1fs", revealTimer), 20, effectY, 24, VIOLET); effectY += 28; }

                DrawTextClear("Move: WASD / Arrow Keys", 20, 732, 20, GRAY);
                DrawTextClear(TextFormat("M: Menu   R: Restart   P: Music %s", musicEnabled ? "OFF" : "ON"), GetScreenWidth() - 382, 732, 20, GRAY);
                break;

            case SCREEN_RESTART_CHOICE:
                // Draw restart choices in the center of the screen.
                DrawTextCentered("RESTART GAME", 95, 58, YELLOW);
                DrawTextCentered(TextFormat("Current Floor: %d", currentFloor), 175, 30, WHITE);

                DrawRectangleLines(GetScreenWidth()/2 - 310, 240, 620, 400, GRAY);
                DrawTextCentered("Choose one option", 285, 26, LIGHTGRAY);
                DrawUIButton((Rectangle){ GetScreenWidth()/2 - 205, 355, 410, 54 }, "Restart from Floor 1", 26, SKYBLUE, WHITE);
                DrawUIButton((Rectangle){ GetScreenWidth()/2 - 205, 430, 410, 54 }, "Restart Current Floor", 26, SKYBLUE, WHITE);
                DrawUIButton((Rectangle){ GetScreenWidth()/2 - 205, 515, 195, 48 }, "Cancel", 23, GRAY, WHITE);
                DrawUIButton((Rectangle){ GetScreenWidth()/2 + 10, 515, 195, 48 }, "Menu", 23, ORANGE, WHITE);
                break;

            case SCREEN_GAMEOVER:
                // Draw final run results and game-over options.
                DrawTextCentered("YOU DIED", 70, 70, RED);
                DrawTextCentered(TextFormat("Player: %s", currentUsername), 155, 24, SKYBLUE);

                if (currentMode == MODE_BASIC) {
                    DrawTextCentered("--- BASIC MODE ---", 235, 24, GREEN);
                    DrawTextCentered(TextFormat("Floor Reached: %d", currentFloor), 285, 36, WHITE);
                    DrawTextCentered(TextFormat("Highest Floor: %d", highScoreBasic), 345, 24, GOLD);
                } else {
                    DrawTextCentered("--- TIMED MODE ---", 235, 24, RED);
                    DrawTextCentered(TextFormat("Floor Reached: %d", currentFloor), 285, 36, WHITE);
                    DrawTextCentered(TextFormat("Highest Floor: %d", highScoreTimed), 345, 24, GOLD);
                }

                {
                    int cx = GetScreenWidth()/2;
                    DrawUIButton((Rectangle){ cx - 145, 420, 290, 44 }, "Restart Game", 23, SKYBLUE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 475, 290, 44 }, "Return to Menu", 23, SKYBLUE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 530, 290, 44 }, "Leaderboard", 23, SKYBLUE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 585, 290, 44 }, "Log Out / Switch", 23, ORANGE, WHITE);
                    DrawUIButton((Rectangle){ cx - 145, 640, 290, 40 }, "Quit Game", 21, RED, WHITE);
                }
                break;
        }

        if (bgmLoaded && ShouldShowMusicMouseButton(currentScreen)) {
            DrawMusicButton(musicEnabled);
        } else if (!bgmLoaded && ShouldShowMusicMouseButton(currentScreen)) {
            DrawTextCentered("Add bgm.mp3 in resources/music/", GetScreenHeight() - 48, 18, GRAY);
        }

        EndDrawing();
    }

    // Persistent Continue save: if the player closes the game from gameplay,
    // restart choice, or game over, remember the current floor for this account + mode.
    if (strcmp(currentUsername, "Guest") != 0 &&
        (currentScreen == SCREEN_GAMEPLAY || currentScreen == SCREEN_RESTART_CHOICE || currentScreen == SCREEN_GAMEOVER)) {
        if (currentFloor > 1) UpdatePlayerBestFloor(currentUsername, currentMode, currentFloor, &highScoreBasic, &highScoreTimed);
        SaveLastFloor(currentUsername, currentMode, currentFloor);
    }

    // Release raylib resources before the program exits.
    if (bgmLoaded) UnloadMusicStream(bgm);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
