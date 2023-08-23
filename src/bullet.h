#pragma once
#include "physics.h"
#include <raylib.h>
#include <stdint.h>

/// Way of addressing a certain bullet on a particular frame. Invalid after
/// frame ends.
typedef struct
{
	uint16_t raw;
} BulletHandle;

typedef enum : uint8_t
{
	PLAYER_ONE = 0,
	PLAYER_TWO,
#ifndef NDEBUG
	PLAYER_NULL,
#endif
} Source;

/// A bullet.
typedef struct
{
	Vector3 position;
	Quaternion direction;
} Bullet;

typedef struct
{
	Bullet bullet;
	Source source;
} BulletCreateOptions;

/// "Create" a new bullet (actually just queues it to be created next frame)
void bullet_create(const BulletCreateOptions* options);

/// Get whether a bullet was fired by player one or two.
/// Implemented in a cache-unfriendly way (doing this will likely read from
/// cold memory)
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

/// Collide and move a set of AABBs, positions, and velocities with the bullets.
/// The collision handler's index_batch1 argument can be cast to a BulletHandle
/// for use with bullet_destroy or bullet_get_source.
/// Do NOT store the bullet handle recieved from the collision handler across
/// frame boundaries. May be invalidated if the bullet is destroyed.
void bullet_move_and_collide_with(
	const AABBBatchOptions* restrict other_aabb,
	const Vector3BatchOptions* restrict other_position,
	const QuaternionBatchOptions* restrict other_direction,
	const FloatBatchOptions* restrict other_speed, CollisionHandler handler);
