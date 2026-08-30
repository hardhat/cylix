// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE.md for the full license text.

#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

struct Player {
    int16_t x;
    int16_t y;
    int8_t dx;
    uint8_t angle; // Angle in 256ths of a circle (0-255)
    int8_t speed;
    int8_t orientation; // which octant the player is facing (0-7)
    int8_t health;
    uint16_t score;
};

// State: high bits are the state, and low 4 bits are a counter
#define ENEMY_STATE_DEAD 0x00
#define ENEMY_STATE_ACTIVE 0x10
#define ENEMY_STATE_COOLDOWN 0x20   // Cooldown after shooting
typedef struct Enemy {
    uint8_t formation;  // Index into the formation table
    uint8_t state;
} Enemy;

// Enemy formation structure
#define FORMATION_PATTERN_SPIRAL 0x01
#define FORMATION_PATTERN_V 0x02
#define FORMATION_PATTERN_LINE 0x03
#define FORMATION_PATTERN_CIRCLE 0x04
#define ENEMY_MAX_COUNT 8
typedef struct Formation {
    uint8_t pattern;
    uint8_t phase;
    uint8_t angle;
    uint8_t z;
    uint8_t count;
    int8_t  angle_speed;
    int8_t  z_speed;
    Enemy enemies[ENEMY_MAX_COUNT];  // Array of enemies in this formation
} Formation;

struct Bullet {
    uint8_t angle;
    uint8_t z;
    bool active;
};

void game_init();
void game_update();
void game_render();
void game_handle_input(uint8_t input, bool down);

#endif // GAME_H