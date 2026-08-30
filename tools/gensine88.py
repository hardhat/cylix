#!/usr/bin/python3

# Copyright (c) 2026 Dale Wick
# SPDX-License-Identifier: MIT
# See LICENSE.md for the full license text.

# Generate a sine table for angles 0-255, where 256 represents a full circle (360 degrees). Values are in 8.8 fixed point format (i.e. 1.0 is represented as 256).
import math

def gensine():
    table = []
    for i in range(256):
        angle = (i / 256) * 2 * math.pi # Convert to radians
        sine_value = int(math.sin(angle) * 256) # Scale to 8.8 fixed point
        table.append(sine_value)
    return table

# Precompute (radius * sin88(angle)) >> 8 for a fixed radius, so fixed-radius objects
# (player, star field) can look up their screen offset directly instead of multiplying
# every frame. cos88(angle) == sin88(angle+64), so callers reuse this same table with a
# +64 index offset instead of needing a second table.
def gen_radius_offset(sine_table, radius):
    return [ (radius * s) >> 8 for s in sine_table ]

# Precompute round(numerator * 2**shift / z) so a runtime division by z can be replaced
# with (value * recip_table[z]) >> shift. Index 0 is unused (division by zero guarded in
# calling code) and left as 0.
def gen_recip_table(numerator=128, shift=8):
    table = [0]
    for z in range(1, 256):
        table.append(round(numerator * (1 << shift) / z))
    return table

def print_table(name, ctype, values):
    print(f"const {ctype} {name}[256] = {{")
    for i in range(0, 256, 8):
        print("    " + ", ".join(f"{values[j]:5d}" for j in range(i, i+8)) + ",")
    print("};")

if __name__ == "__main__":
    print("// Copyright (c) 2026 Dale Wick")
    print("// SPDX-License-Identifier: MIT")
    print("// See LICENSE.md for the full license text.")
    print()
    sine_table = gensine()
    print("uint16_t g_sine_table[256] = {")
    for i in range(0, 256, 8):
        print("    " + ", ".join(f"{sine_table[j]:5d}" for j in range(i, i+8)) + ",")
    print("};")
    print()
    print_table("g_player_offset", "int16_t", gen_radius_offset(sine_table, 90))
    print()
    print_table("g_star_offset", "int16_t", gen_radius_offset(sine_table, 80))
    print()
    print_table("g_recip128_table", "uint16_t", gen_recip_table(128, 8))
