#include <stdlib.h>

#include "main.h"
#include "game.h"
#include "sin88.h"

struct Player player;
#define BULLET_COUNT 16
struct Bullet bullets[BULLET_COUNT];

// Star field background
#define STAR_SPRITE_INDEX 17 // Assuming the star sprite is at index 17
#define STAR_COUNT 16 // Number of stars in the star field
#define STAR_RADIUS 80 // Fixed orbit radius for stars before perspective projection
uint8_t star_angle[STAR_COUNT];
int16_t star_z[STAR_COUNT];
int star_count = STAR_COUNT;



const struct EnemySprite {
    uint8_t index[3];       // Tile index for ship pointing up, up-right, and right
} enemy_sprite[4] = {
    {{0xF,0x1F,0x2F}},    // Distance 0-63 (smallest)
    {{0xE,0x1E,0x2E}},    // Distance 64-127
    {{0xD,0x1D,0x2D}},    // Distance 128-191
    {{0xC,0x1C,0x2C}},    // Distance 192-255 (Largest)
};

Formation formation;

void game_init() {
    // Initialize player
    player.x = 32;
    player.y = 240-64;
    player.dx = 0;
    player.angle = 64; // Bottom of the screen, facing up
    player.orientation = 0; // Facing up
    player.speed = 4;
    player.health = 100;
    player.score = 0;
    // Initialize star field
    for(int i=0;i<star_count;i++) {
        star_angle[i] = rand() % 256; // Random angle in range [0, 255]
        star_z[i] = rand() % 256 + 1; // Random z position in range [1, 256]
    }

    const uint8_t stage_map[4]={49,50,51,33};
    const uint8_t cylix_map[3]={52,53,54};
    uint8_t lives_map[3]={48,48,48};

    uint8_t line[20];
    for(int i=0;i<20;i++) line[i]=0;
    for(int y=0;y<15;y++) show_map_xy(line,20,1,0,y);

    show_map_xy(stage_map,4,1,12,0);
    show_map_xy(cylix_map,3,1,0,0);
    show_map_xy(lives_map,3,1,0,14);

    formation.pattern = FORMATION_PATTERN_SPIRAL;
    formation.phase = 0;
    formation.angle = 0;
    formation.z = 0;
    formation.count = ENEMY_MAX_COUNT;
    formation.angle_speed = 8;
    formation.z_speed = 4;
    for(int i=0;i<ENEMY_MAX_COUNT;i++) {
        formation.enemies[i].formation = 0;
        formation.enemies[i].state = ENEMY_STATE_ACTIVE;
    }
}

void game_update() {
    // Update game logic here
    player.angle = (player.angle + player.dx) % 256; // Rotate player angle
    player.x = ((90 * sin88(player.angle))>>8) + 160; // Move player in a circular path
    player.y = ((90 * cos88(player.angle))>>8) + 120; // Move player in a circular path
    player.orientation = ((player.angle+16) >> 5) & 0x07; // Determine orientation based on angle
    // Update formation angle and z position
    formation.angle = (formation.angle + formation.angle_speed) % 256;
    formation.z += formation.z_speed;

    // Additional game update code here
}

void add_enemy(uint8_t angle, uint8_t z) {
    // Calculate the orientation
    uint8_t orientation = ((angle+16) >> 5) & 0x07; // Determine orientation based on angle
    // Determine the flags
    uint8_t enemy_size = z>>6; // Determine enemy size based on distance (z)
    // Frames are: 2,3,18,19=up, 4,5,20,21=up-right, 6,7,22,23=right - then mirror for rest.
    bool corner = orientation & 0x01; // Check if orientation is a corner (odd)
    bool vertical = (orientation & 0x02) == 0; // Check if orientation is vertical (0 or 1)
    uint8_t enemy_index = enemy_sprite[enemy_size].index[(corner ? 2 : (vertical ? 0 : 4))];  // orientation
    // Flip flags per orientation don't follow a clean bit pattern, so use a lookup table
    static const uint8_t orientation_flags[8] = {
        SPRITE_FLAG_NONE,
        SPRITE_FLAG_FLIP_X,
        SPRITE_FLAG_FLIP_X,
        SPRITE_FLAG_FLIP_Y | SPRITE_FLAG_FLIP_X,
        SPRITE_FLAG_FLIP_Y,
        SPRITE_FLAG_FLIP_Y,
        SPRITE_FLAG_NONE,
        SPRITE_FLAG_NONE,
    };
    uint8_t flags = orientation_flags[orientation];
    // Set the sprite for the enemy based on its orientation and index
    add_sprite((((z>>1) * sin88(angle))>>8) + 160, (((z>>1) * cos88(angle))>>8) + 120, enemy_index, flags);
}

int active_enemy_count() {
    int count = 0;
    for(int i=0;i<formation.count;i++) {
        if(formation.enemies[i].state == ENEMY_STATE_ACTIVE) {
            count++;
        }
    }
    return count;
}

void add_enemy_formation() {
    // Add enemies based on the current formation
    for(int i=0;i<formation.count;i++) {
        if(formation.enemies[i].state == ENEMY_STATE_ACTIVE) {
            // Calculate the angle and z position for the enemy
            uint8_t angle = (formation.angle + (i * 32)) % 256; // Spread enemies around the formation angle
            uint8_t z = formation.z + (i * 16); // Spread enemies in depth
            add_enemy(angle, z);
        }
    }
}

void game_render() {
    reset_sprite();
    // Render game graphics here (anchor in bottom center)

    // Star field background -- make stars move towards you with a vanishing point in the center of the screen
    for(int i=0;i<star_count;i++) {
        uint8_t angle = star_angle[i];
        int16_t z = star_z[i];

        // Move the star towards the player
        z -= 4;
        if(z <= 0) {
            // Reset the star to a new random angle
            z = 200; //rand() % 256 + 1; // Avoid division by zero
            angle = rand() % 256;
        }

        // Compute the star's position on a fixed-radius orbit
        int16_t x = (STAR_RADIUS * sin88(angle)) >> 8;
        int16_t y = (STAR_RADIUS * cos88(angle)) >> 8;

        // Project the star's position onto the screen
        int16_t screen_x = (x * 128) / z + 160; // Centered at (160, 120)
        int16_t screen_y = (y * 128) / z + 120;

        // Draw the star if it's within the screen bounds
        if(screen_x >= 0 && screen_x < 320 && screen_y >= 0 && screen_y < 240) {
            add_sprite(screen_x, screen_y, STAR_SPRITE_INDEX, SPRITE_FLAG_NONE);
        }

        // Update the star's position
        star_angle[i] = angle;
        star_z[i] = z;
    }

    for(uint8_t i=0;i<BULLET_COUNT;i++) {
        if(bullets[i].active) {
            // Update bullet position based on its angle
            bullets[i].z -= 4; // Move bullet forward
            if(bullets[i].z <= 0) {
                bullets[i].active = false; // Deactivate bullet if it goes off-screen
            } else {
                int16_t bullet_x = (bullets[i].z * sin88(bullets[i].angle)) >> 8;
                int16_t bullet_y = (bullets[i].z * cos88(bullets[i].angle)) >> 8;
                add_sprite(160 + bullet_x, 120 + bullet_y, 1, SPRITE_FLAG_NONE); // Assuming bullet sprite is at index 16
            }
        }
    }

    // Frames are: 2,3,18,19=up, 4,5,20,21=up-right, 6,7,22,23=right - then mirror for rest.
    bool corner = player.orientation & 0x01; // Check if orientation is a corner (odd)
    bool vertical = (player.orientation & 0x02) == 0; // Check if orientation is vertical (0 or 1)
    int base_frame = 2 + (corner ? 2 : (vertical ? 0 : 4));  // orientation
    // Flip flags per orientation don't follow a clean bit pattern, so use a lookup table
    static const uint8_t orientation_flags[8] = {
        SPRITE_FLAG_NONE,
        SPRITE_FLAG_FLIP_X,
        SPRITE_FLAG_FLIP_X,
        SPRITE_FLAG_FLIP_Y | SPRITE_FLAG_FLIP_X,
        SPRITE_FLAG_FLIP_Y,
        SPRITE_FLAG_FLIP_Y,
        SPRITE_FLAG_NONE,
        SPRITE_FLAG_NONE,
    };
    uint8_t flags = orientation_flags[player.orientation];
    bool flip_x = (flags & SPRITE_FLAG_FLIP_X) != 0;
    bool flip_y = (flags & SPRITE_FLAG_FLIP_Y) != 0;
    add_sprite(player.x-16, player.y-16, base_frame + (flip_x ? 1 : 0) + (flip_y ? 16 : 0), flags);
    add_sprite(player.x, player.y-16, base_frame + (flip_x ? 0 : 1) + (flip_y ? 16 : 0), flags);
    add_sprite(player.x-16, player.y, base_frame + (flip_x ? 1 : 0) + (flip_y ? 0 : 16), flags);
    add_sprite(player.x, player.y, base_frame + (flip_x ? 0 : 1) + (flip_y ? 0 : 16), flags);

    add_enemy_formation();

    render_sprites();

    show_number(player.score, 15, 14);
    show_number(active_enemy_count(), 15, 13);
}

void game_handle_input(uint8_t input, bool down) {
    // Handle game input here
    if(input == INPUT_UP && down) {
        // Move player up
    } else if(input == INPUT_DOWN && down) {
        // Move player down
    } else if(input == INPUT_LEFT && down) {
        // Move player left
        player.dx = -player.speed;
        //debug_log("left");
    } else if(input == INPUT_RIGHT && down) {
        // Move player right
        player.dx = player.speed;
        //debug_log("right");
    } else if(input == INPUT_LEFT && !down) {
        // Stop moving left
        player.dx = 0;
        //debug_log("stop left");
    } else if(input == INPUT_RIGHT && !down) {
        // Stop moving right
        player.dx = 0;
        //debug_log("stop right");
    }
    if(input == INPUT_A && down) {
        // Fire bullet
        for(uint8_t i=0;i<BULLET_COUNT;i++) {
            if(!bullets[i].active) {
                bullets[i].active = true;
                bullets[i].angle = player.angle;
                bullets[i].z = 80; // Start bullet at a distance
                break;
            }
        }
    }
}