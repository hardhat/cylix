// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE.md for the full license text.

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>

enum GameState {
    STATE_MENU,
    STATE_GAME,
    STATE_EXIT
};

enum INPUT
{
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_A,
    INPUT_B,
    INPUT_X,
    INPUT_Y,
    INPUT_START,
    INPUT_SELECT,
    INPUT_L,
    INPUT_R,
    MAX_INPUT
};

#define SPRITE_FLAG_NONE 0x00
#define SPRITE_FLAG_PRIORITY 0x02
#define SPRITE_FLAG_FLIP_Y 0x04
#define SPRITE_FLAG_FLIP_X 0x08

extern enum GameState current_state;

void set_game_state(enum GameState new_state);
/// Reset the off-screen sprites table to empty
void reset_sprite(void);
/// Render all sprites to the screen from the off-screen sprites table
void render_sprites(void);
/// Clear all sprites from the screen and off-screen sprites table
void clear_sprites(void);
/// Add a sprite to the screen with the given parameters
/// @param x The x-coordinate of the sprite
/// @param y The y-coordinate of the sprite
/// @param tile The tile index of the sprite
/// @param flags The flags for the sprite (e.g., priority, flip)
/// @return The index of the added sprite, or 255 if the sprite table is full
uint8_t add_sprite(uint16_t x, uint8_t y, uint8_t tile, uint16_t flags);
/// Show map on layer 0
void show_map(uint8_t *map,uint8_t width,uint8_t height);
/// @brief Show number on the screen at the specified tilemap coordinates
/// @param  number The number to display
/// @param  x The x-coordinate on the tilemap
/// @param  y The y-coordinate on the tilemap
void show_number(uint16_t number, uint8_t x, uint8_t y);
/// @brief Show a portion of the map on the screen at the specified tilemap coordinates
/// @param map The map data to display
/// @param width The width of the map in tiles
/// @param height The height of the map in tiles
/// @param x The x-coordinate on the tilemap
/// @param y The y-coordinate on the tilemap
void show_map_xy(uint8_t *map,uint8_t width,uint8_t height,uint8_t x,uint8_t y);
/// @brief Log a message to the debug output
void debug_log(const char *message);
/// @brief  Log a formatted message to the debug output
/// @param format The format string (printf-style)
/// @param ... The values to format
void debug_logf(const char *format, ...);

#endif // MAIN_H
