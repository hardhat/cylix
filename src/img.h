// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE.md for the full license text.

#ifndef IMG_H
#define IMG_H

#include <stdint.h>

extern const uint16_t ship_palette[64];
extern const uint16_t ship_palette_len;
#define SHIP_PALETTE_BASE 0
extern const uint8_t *ship_tiles;
#define SHIP_TILES_BASE 0
extern const uint16_t ship_tiles_len;

#endif // IMG_H