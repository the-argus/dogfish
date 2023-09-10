#pragma once
#include <raylib.h>
#include <stdint.h>

void terrain_draw();

/// Potentially load new chunks and unload old ones.
void terrain_update();

// Load terrain. Assumes that the players start near 0x0x0, otherwise may load
// in a bunch of terrain only to unload it when terrain_update_player_pos is
// called.
void terrain_load();

/// Ask the terrain to unload distant chunks and load in nearby chunks. Loading
/// may not occur if the new position of the player is too similar to the
/// previous.
/// @param index: number less than NUM_PLANES
/// @param pos: the position the player at that index has moved to
void terrain_update_player_pos(uint8_t index, Vector3 pos);

void terrain_cleanup();
