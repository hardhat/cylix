# Tube Shooter Enemy & Formation System Specification

**Target platform:** Zeal 8-bit Computer  
**CPU:** Z80 @ 10 MHz  
**Game style:** Gyruss-inspired tube space shooter  
**Primary goal:** Support large numbers of enemies and visually complex formations while keeping CPU and memory requirements appropriate for an 8-bit Z80.

---

## 1. Design Goals

The enemy system should:

- Represent enemy positions compactly.
- Avoid floating-point arithmetic.
- Avoid general-purpose 3D calculations.
- Use 8-bit angles for inexpensive rotational movement.
- Use lookup tables wherever practical.
- Support large enemy swarms.
- Allow complex enemy formations to be described with very little data.
- Separate formation movement from individual enemy state.
- Allow approximately 60–80 visible enemies while remaining well below the Zeal 128-sprite hardware limit.
- Make collision detection inexpensive.
- Support scripted arcade-style enemy waves.

The system should create the appearance of a fully 3D tube while actually operating primarily in a two-dimensional coordinate system:

```text
Enemy position = (angle, z)
```

where `angle` determines position around the tube and `z` determines apparent distance from the player.

---

# 2. Coordinate System

## 2.1 Angular Coordinate

Use an unsigned 8-bit value for a complete revolution:

```c
uint8_t angle;
```

Mapping:

| Angle | Degrees |
|---:|---:|
| 0 | 0° |
| 64 | 90° |
| 128 | 180° |
| 192 | 270° |
| 255 | ~358.6° |

One full revolution is therefore exactly 256 units.

### Advantages

Angular movement becomes:

```c
angle += angular_velocity;
```

No special wrapping operation is required.

Overflow naturally wraps:

```text
255 + 1 = 0
```

This should be used throughout the game for all circular coordinates.

---

# 3. Player Position

The player moves around a fixed-radius circle.

```c
uint8_t player_angle;
```

The nominal player radius is:

```c
#define PLAYER_RADIUS 80
```

Screen coordinates are calculated from the existing fixed-point sine/cosine functions.

Conceptually:

```c
x = CENTER_X + sin88(player_angle) * PLAYER_RADIUS;
y = CENTER_Y - cos88(player_angle) * PLAYER_RADIUS;
```

The exact shift required depends on the fixed-point representation used by `sin88()` and `cos88()`.

The player therefore requires only one byte of positional state.

---

# 4. Enemy Depth (`z`)

Each enemy may be represented by:

```c
uint8_t z;
```

`z` represents position along the tube rather than conventional Cartesian depth.

Recommended convention:

```text
z = 0       farthest
z = 255     closest
```

The exact direction may be reversed if it makes the implementation easier.

Enemy movement therefore becomes:

```c
enemy.z += enemy.z_velocity;
```

or, when controlled by a formation:

```c
formation.z += formation.z_velocity;
```

---

# 5. Perspective Projection

Do not perform a conventional 3D perspective calculation.

Instead, use a lookup table:

```c
uint8_t radius_table[256];
```

The table converts `z` into the apparent screen radius.

Conceptually:

```c
radius = radius_table[z];
```

The radius table should be tuned artistically rather than necessarily representing physically correct perspective.

Example:

```text
z              radius

0              4
32             8
64             14
96             20
128            27
160            37
192            48
224            63
255            80
```

These values are illustrative and should be tuned during implementation.

---

# 6. Screen Projection

Once `radius` is obtained:

```c
x = CENTER_X + ((sin_table[angle] * radius) >> FIXED_SHIFT);
y = CENTER_Y - ((cos_table[angle] * radius) >> FIXED_SHIFT);
```

Where:

```c
sin_table[256]
cos_table[256]
```

are lookup tables containing the existing fixed-point sine/cosine values.

The projection therefore consists primarily of:

1. Two table lookups.
2. A radius table lookup.
3. Two integer multiplications.
4. Two shifts.
5. Two additions.

No floating-point arithmetic should be required.

---

# 7. Sprite Size

Enemy apparent size should also be derived from `z`.

A second lookup table may be used:

```c
uint8_t size_table[256];
```

Alternatively, divide `z` into ranges:

```c
size_class = z >> 4;
```

Possible size classes:

| z range | Appearance |
|---|---|
| 0–63 | tiny/distant |
| 64–127 | small |
| 128–191 | medium |
| 192–255 | large/close |

The graphical representation can use different sprite graphics or Zeal sprite dimensions where appropriate.

The system should prioritize keeping the number of actual hardware sprites within the available sprite budget.

---

# 8. Basic Enemy State

A fully independent enemy can be represented as:

```c
typedef struct {
    uint8_t angle;
    uint8_t z;
    uint8_t type;
    uint8_t state;
} Enemy;
```

This is a maximum of 4 bytes per enemy before any additional game-specific state is added.

However, this should **not** be the default representation for ordinary formation enemies.

Most enemies should derive their position from a formation.

---

# 9. Formation-Based Enemy Architecture

The game should separate the simulation into three levels:

```text
Stage Script
     |
     v
Formation
     |
     v
Enemy
```

## 9.1 Stage

Controls:

- When formations appear.
- Which formation appears.
- Formation timing.
- Number of formations.
- Special events.
- Boss encounters.
- Background/gameplay events.

## 9.2 Formation

Controls:

- Overall position.
- Movement.
- Rotation.
- Pattern.
- Number of enemies.
- Pattern phase.
- Formation velocity.

## 9.3 Enemy

Stores only properties that are genuinely individual:

- Enemy type.
- State.
- Alive/dead.
- Health, if required.
- Animation state.
- Special status.

Enemy position does not necessarily need to be stored.

---

# 10. Formation Structure

Initial implementation target:

```c
typedef struct {
    uint8_t pattern;
    uint8_t phase;
    uint8_t angle;
    uint8_t z;
    uint8_t count;
    int8_t  angle_speed;
    int8_t  z_speed;
} Formation;
```

This structure is intentionally compact.

A formation can contain many enemies while requiring only a handful of bytes of motion state.

Each frame:

```c
formation.angle += formation.angle_speed;
formation.z     += formation.z_speed;
```

---

# 11. Pattern System

A formation should reference a pattern by ID:

```c
uint8_t pattern;
```

Initial pattern types:

```c
enum Pattern {
    PATTERN_LINE,
    PATTERN_RING,
    PATTERN_V,
    PATTERN_SPIRAL,
    PATTERN_CROSS,
    PATTERN_SWARM,
    PATTERN_DOUBLE_SWARM
};
```

The actual implementation may use a more compact or data-driven representation.

---

# 12. Pattern Offsets

Patterns describe enemy positions relative to the formation origin.

For example, a ring can use angular offsets:

```text
0
32
64
96
128
160
192
224
```

All enemies can share the same formation `z`.

The resulting positions are:

```c
enemy_angle = formation.angle + angle_offset[i];
enemy_z     = formation.z + z_offset[i];
```

Because `angle` and `z` are 8-bit values, addition remains inexpensive.

---

# 13. Parameterized Patterns

Whenever possible, patterns should be generated from parameters rather than storing individual coordinates.

Example:

```c
typedef struct {
    uint8_t count;
    int8_t  angle_spacing;
    int8_t  z_spacing;
    int8_t  angle_velocity;
    int8_t  z_velocity;
} Pattern;
```

A line formation could therefore be represented by:

```text
count           = 16
angle_spacing   = 0
z_spacing       = 10
```

A spiral might use:

```text
count           = 24
angle_spacing   = 8
z_spacing       = 3
angle_velocity  = 2
z_velocity      = -2
```

This allows many formations to be created from a very small amount of data.

---

# 14. Pattern Phase

Patterns should support a phase value:

```c
uint8_t phase;
```

The phase allows a formation to evolve over time.

For example:

```text
Phase 0:

          E
          E
          E
          E


Phase 1:

         E E
        E   E
         E E


Phase 2:

       E       E
         E   E
           E
```

A single pattern can therefore produce multiple stages of movement without requiring a separate formation for every phase.

---

# 15. Formation Splitting

The pattern system should eventually support formations that split and recombine.

Example:

```text
            E
            E
            E
            E
            E
```

becomes:

```text
          E   E
        E       E
      E           E
```

and eventually:

```text
     E                 E
       E             E
         E         E
           E     E
```

This can be implemented by changing pattern parameters or phase rather than physically moving every enemy independently.

---

# 16. Example Formation Types

The initial game should support at least the following formation concepts.

### Single File

```text
E
E
E
E
E
E
```

### Horizontal/Angular Line

```text
E E E E E E
```

### Ring

```text
      E

  E       E

E           E

  E       E

      E
```

### V

```text
E           E
 E         E
  E       E
   E     E
    E   E
     E E
      E
```

### Spiral

Enemies progressively change their angular offset and depth.

### Crossing Swarms

Two or more groups move through one another.

### Split Formation

A single group separates into two or more groups.

### Recombining Formation

Previously separated groups converge again.

---

# 17. Collision Detection

Because the tube coordinate system is already polar, collision detection can be simplified considerably.

For a player collision:

```c
d = enemy.angle - player.angle;

if (d > 128)
    d = 256 - d;
```

Then test:

```c
if (enemy.z >= COLLISION_Z &&
    d < COLLISION_ANGLE)
{
    // collision
}
```

This avoids calculating Cartesian distance.

The exact collision thresholds should be tuned experimentally.

---

# 18. Enemy Visibility

Simulation and rendering should be treated as separate systems.

An enemy may exist in the simulation without currently requiring a hardware sprite.

Potential reasons:

- Too far away to be visible.
- Behind another object.
- Outside the visible area.
- Formation is transitioning.
- Sprite budget is exhausted.
- Enemy is not currently relevant to gameplay.

The renderer should select which enemies receive hardware sprites.

---

# 19. Sprite Budget

The Zeal hardware supports up to 128 sprites.

The game should nevertheless establish a practical target below this limit.

Initial target:

```text
60–80 visible enemies maximum
```

The remaining sprite capacity can be reserved for:

- Player.
- Player projectiles.
- Enemy projectiles.
- Effects.
- Bosses.
- UI elements where necessary.

The renderer must also account for per-scanline sprite limitations.

---

# 20. Level of Detail

Distant enemies do not necessarily need to be updated every frame.

Potential optimization:

```text
z = 0–63       update every 4 frames
z = 64–127     update every 2 frames
z = 128–255    update every frame
```

This should only be introduced if profiling demonstrates that it is useful.

A simpler first implementation should update all active formations every frame.

---

# 21. Global Formation Simulation

Where possible, movement should be performed at the formation level.

Instead of:

```c
for every enemy:
    enemy.angle += enemy.angular_velocity;
    enemy.z += enemy.z_velocity;
```

prefer:

```c
formation.angle += formation.angle_speed;
formation.z += formation.z_speed;
```

followed by deriving enemy positions:

```c
enemy_angle = formation.angle + pattern_angle_offset;
enemy_z     = formation.z + pattern_z_offset;
```

This dramatically reduces the amount of mutable state.

---

# 22. Recommended Processing Pipeline

Each game frame should conceptually execute:

```text
1. Read player controls
        |
2. Update player angle
        |
3. Update formation state
        |
4. Advance pattern phases
        |
5. Generate enemy positions
        |
6. Perform collision tests
        |
7. Select visible enemies
        |
8. Project enemies to screen coordinates
        |
9. Determine sprite graphics/size
        |
10. Write sprite attributes
```

The implementation should be profiled to determine the actual optimal ordering.

---

# 23. Memory Strategy

Favor tables and compact structures.

Likely persistent tables:

```c
uint8_t sin_table[256];
uint8_t cos_table[256];

uint8_t radius_table[256];

uint8_t size_table[256];       // optional
```

Patterns should preferably be stored as compact data rather than large lists of full enemy coordinates.

Avoid storing redundant values such as:

```text
screen_x
screen_y
radius
angle
z
```

for every enemy.

Screen coordinates should be generated when rendering.

---

# 24. CPU Optimization Principles

The implementation should prioritize:

- 8-bit arithmetic.
- Incremental updates.
- Lookup tables.
- Formation-level movement.
- Pattern parameters.
- Avoiding division.
- Avoiding floating point.
- Avoiding general-purpose multiplication where a lookup or shift is practical.
- Avoiding repeated calculations of values shared by a formation.

The engine should be written so that optimization can be guided by profiling rather than premature assumptions.

---

# 25. MVP Implementation

The first working version should **not** attempt to implement every pattern.

MVP:

1. Player rotates around the tube.
2. One formation enters the tube.
3. Formation contains multiple enemies.
4. Enemies have `angle` and `z` positions.
5. Perspective uses `radius_table`.
6. Enemy positions use the existing sine/cosine lookup.
7. Enemy sprites become larger as they approach.
8. Player/enemy collision works using angular distance.
9. Formation movement is controlled by shared velocity.
10. At least three formation patterns are supported:
    - line
    - ring
    - spiral

Once this is working and profiled, add more sophisticated patterns.

---

# 26. TODO Checklist

## Coordinate System

- [ ] Define exact `angle` convention.
- [ ] Confirm `0–255 = 0–360°`.
- [ ] Define exact `z` direction.
- [ ] Define player radius.
- [ ] Define tube center coordinates.
- [ ] Confirm fixed-point format used by `sin88()` and `cos88()`.

## Lookup Tables

- [ ] Generate 256-entry sine table.
- [ ] Generate 256-entry cosine table.
- [ ] Generate initial `radius_table[256]`.
- [ ] Tune perspective curve visually.
- [ ] Determine whether a separate sprite-size table is necessary.
- [ ] Measure ROM/RAM cost of all lookup tables.

## Player

- [ ] Implement `player_angle`.
- [ ] Implement player projection.
- [ ] Implement player movement speed.
- [ ] Implement angular wrap-around.
- [ ] Render player sprite.

## Basic Enemies

- [ ] Implement basic enemy structure.
- [ ] Implement enemy type.
- [ ] Implement enemy alive/dead state.
- [ ] Implement enemy angle.
- [ ] Implement enemy z.
- [ ] Implement basic enemy projection.
- [ ] Implement enemy sprite rendering.
- [ ] Test maximum number of active enemies.

## Formation System

- [ ] Implement `Formation` structure.
- [ ] Implement formation angle.
- [ ] Implement formation z.
- [ ] Implement formation angular velocity.
- [ ] Implement formation z velocity.
- [ ] Implement formation count.
- [ ] Implement formation activation/deactivation.
- [ ] Implement formation lifecycle.

## Pattern System

- [ ] Define pattern ID format.
- [ ] Implement line pattern.
- [ ] Implement ring pattern.
- [ ] Implement V pattern.
- [ ] Implement spiral pattern.
- [ ] Implement pattern angle offsets.
- [ ] Implement pattern z offsets.
- [ ] Implement parameterized patterns.
- [ ] Implement pattern phase.
- [ ] Implement pattern transitions.
- [ ] Implement split formations.
- [ ] Implement recombining formations.
- [ ] Implement crossing formations.

## Collision

- [ ] Implement 8-bit angular difference.
- [ ] Implement collision angular threshold.
- [ ] Implement collision z threshold.
- [ ] Test collisions at 0°/255° boundary.
- [ ] Test collisions with multiple enemies.
- [ ] Add enemy projectile collision later.

## Rendering

- [ ] Implement `radius_table` projection.
- [ ] Implement screen X calculation.
- [ ] Implement screen Y calculation.
- [ ] Determine minimum visible radius.
- [ ] Determine maximum visible radius.
- [ ] Implement sprite size selection.
- [ ] Implement sprite prioritization.
- [ ] Implement sprite-budget handling.
- [ ] Test scanline sprite limitations.
- [ ] Reserve sprite capacity for player/projectiles/effects.

## Performance

- [ ] Profile formation update cost.
- [ ] Profile pattern generation cost.
- [ ] Profile sine/cosine lookup cost.
- [ ] Profile projection cost.
- [ ] Profile sprite generation cost.
- [ ] Measure maximum enemies at 60 FPS.
- [ ] Determine whether distant-enemy update throttling is necessary.
- [ ] Optimize only after profiling.

## Stage/Wave System

- [ ] Define stage script format.
- [ ] Implement formation spawn command.
- [ ] Implement formation delay.
- [ ] Implement formation movement parameters.
- [ ] Implement pattern selection.
- [ ] Implement pattern phase events.
- [ ] Implement formation destruction.
- [ ] Implement wave completion.
- [ ] Implement boss/large formation support.

## Testing

- [ ] Test 1 enemy.
- [ ] Test 8 enemies.
- [ ] Test 16 enemies.
- [ ] Test 32 enemies.
- [ ] Test 64 enemies.
- [ ] Test 80+ enemies.
- [ ] Test multiple simultaneous formations.
- [ ] Test enemies crossing the 0°/255° boundary.
- [ ] Test enemies entering/leaving the tube.
- [ ] Test large numbers of sprites.
- [ ] Measure CPU usage.
- [ ] Measure memory usage.
- [ ] Verify stable frame rate.

---

# 27. Future Enhancements

Once the core system is working, consider:

- Formation scripting language.
- Formation chaining.
- Formation splitting/recombining.
- Enemies that leave the formation and become independently simulated.
- Enemy-specific movement after reaching the player plane.
- Homing enemies.
- Spiral attack patterns.
- Enemies that orbit the tube.
- Boss formations.
- Procedurally generated formations.
- Pattern randomization.
- Enemy animation synchronized with `z`.
- Multiple depth layers.
- Fake tube rotation.
- Background stars synchronized with tube movement.

---

# 28. Core Design Principle

The fundamental design principle for the engine is:

> **Simulate formations, not individual enemies, whenever possible.**

An apparently complex swarm such as:

```text
          E       E
       E     E E     E
     E   E       E   E
       E     E E     E
          E       E
```

should ideally be represented by something closer to:

```text
Formation {
    pattern = SWARM;
    phase = 37;
    angle = 92;
    z = 140;
    count = 32;
    angle_speed = 2;
    z_speed = -3;
}
```

The renderer expands that compact description into individual sprite positions only when necessary.

This allows the game to produce the visual impression of a large, highly dynamic 3D enemy swarm while keeping the underlying simulation extremely inexpensive for the 10 MHz Z80.