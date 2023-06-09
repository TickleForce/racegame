#pragma once

enum
{
    COLLISION_FLAG_TRACK      = 1 << 0,
    COLLISION_FLAG_TERRAIN    = 1 << 1,
    COLLISION_FLAG_OBJECT     = 1 << 2,
    COLLISION_FLAG_WHEEL      = 1 << 3,
    COLLISION_FLAG_CHASSIS    = 1 << 4,
    COLLISION_FLAG_DEBRIS     = 1 << 5,
    COLLISION_FLAG_SELECTABLE = 1 << 6,
    COLLISION_FLAG_DUST       = 1 << 7,
    COLLISION_FLAG_OIL        = 1 << 8,
    COLLISION_FLAG_GLUE       = 1 << 9,
    COLLISION_FLAG_MINE       = 1 << 10,
    COLLISION_FLAG_DYNAMIC    = 1 << 11,
    COLLISION_FLAG_PICKUP     = 1 << 12,
    COLLISION_FLAG_BOOSTER    = 1 << 13,
    COLLISION_FLAG_SPLINE     = 1 << 14,
};

enum
{
    DECAL_NONE        = 0,
    DECAL_TRACK       = 1 << 0,
    DECAL_RAILING     = 1 << 1,
    DECAL_TERRAIN     = 1 << 2,
    DECAL_GROUND      = 1 << 3,
    DECAL_VEGETATION  = 1 << 4,
    DECAL_SIGN        = 1 << 5,
    DECAL_PLACEHOLDER = 1 << 6,
};

enum
{
    DRIVABLE_SURFACE = 0xffff0000,
    UNDRIVABLE_SURFACE = 0x0000ffff
};
