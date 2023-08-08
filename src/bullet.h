#pragma once
#include "physics.h"
#include <raylib.h>
#include <stdint.h>

typedef uint16_t BulletHandle;

typedef enum : uint8_t
{
	PLAYER_ONE = 0,
	PLAYER_TWO,
} Source;

/// "Create" a new bullet (actually just queues it to be created)
/// The direction is the direction of the bullet relative to the up vector
void bullet_create(const Vector3* restrict position,
				   const Quaternion* restrict direction, Source source);
Source bullet_get_source(BulletHandle bullet);
/// Destroy a bullet (queues it for destruction)
void bullet_destroy(BulletHandle bullet);
/// actually create or destroy bullets queued by bullet_create or
/// bullet_destroy. may cause allocation
void bullet_update();
/// draw all bullets
void bullet_draw();
/// initialize bullet data
void bullet_init();
/// free memory and clean up
void bullet_cleanup();

// physics. inlined

extern AABBBatchOptions bullet_data_aabb_options;
extern Vector3BatchOptions bullet_data_position_options;
extern QuaternionBatchOptions bullet_data_velocity_options;
extern ByteBatchOptions bullet_data_disabled_options;

/// Collide and move a set of AABBs, positions, and velocities with the bullets.
/// The collision handler's index_batch1 argument can be cast to a BulletHandle
/// for use with bullet_destroy.
inline void bullet_move_and_collide_with(
	const AABBBatchOptions* restrict other_aabb,
	const Vector3BatchOptions* restrict other_position,
	const QuaternionBatchOptions* restrict other_velocity,
	CollisionHandler handler)
{
	assert((void*)other_position != (void*)other_velocity);
	assert((void*)other_aabb != (void*)other_velocity);
	assert((void*)other_aabb != (void*)other_position);
	// TODO: maybe try swapping the bullet data to be the second batch so it's
	// in the inner loop. has performance implications.
	// TODO: consider using point-to-aabb collisions for bullets, may be cheaper
	physics_batch_collide_and_move(
		&bullet_data_aabb_options, other_aabb, &bullet_data_position_options,
		other_position, &bullet_data_velocity_options, other_velocity,
		&bullet_data_disabled_options, NULL, handler);
}
