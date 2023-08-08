#pragma once
#include "physics.h"
#include <stdint.h>

typedef uint8_t PlaneHandle;

/// Initialize all plane-related resources
void airplane_init();
/// Update all planes.
void airplane_update(float delta_time);
/// Draw all planes.
void airplane_draw();
/// unload and deallocate plane resources
void airplane_cleanup();

extern AABBBatchOptions airplane_data_aabb_options;
extern Vector3BatchOptions airplane_data_position_options;
extern QuaternionBatchOptions airplane_data_velocity_options;
