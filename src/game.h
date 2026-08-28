#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

struct Player {
    int x;
    int y;
    int dx,dy;
    int angle; // Angle in 256ths of a circle (0-255)
    int speed;
    int orientation; // which octant the player is facing (0-7)
    int health;
    int score;
};

struct Bullet {
    uint8_t angle;
    int z;
    bool active;
};

void game_init();
void game_update();
void game_render();
void game_handle_input(uint8_t input, bool down);

#endif // GAME_H