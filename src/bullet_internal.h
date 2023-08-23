#pragma once
///
/// Declarations only used by the bullet module
///

#include "bullet.h"
#include <raylib.h>
#include <stdint.h>

#define BULLET_PHYSICS_WIDTH 0.1f
#define BULLET_PHYSICS_LENGTH 0.5f
#define BULLET_MASS 1
#define BULLET_POOL_SIZE_INITIAL 256
#define BULLET_POOL_MAX_POSSIBLE_COUNT 65535
// maximum number of bullets that can be created/destroyed until update is
// called (ie. per frame)
#define BULLET_STACKS_MAX_SIZE 32
// hoe many update calls should happen before looping through bullets and
// disabling
#define CALLS_PER_DESPAWN 20
// time in seconds before a bullet is considered old and in need of freeing
#define DESPAWN_TIME_SECONDS 2.0f
// vector reallocation coefficient
#define BULLET_ALLOCATION_SCALE_FACTOR 2

typedef struct
{
	uint16_t count;
	uint16_t capacity;
	Source* sources; // the source of each bullet, ie PLAYER_ONE or TWO
	double* create_times;
	Bullet items[0];
} BulletData;

// stack types
typedef struct
{
	uint16_t count;
	BulletCreateOptions stack[BULLET_STACKS_MAX_SIZE];
} BulletCreationStack;
typedef struct
{
	uint16_t count;
	BulletHandle stack[BULLET_STACKS_MAX_SIZE];
} BulletDestructionStack;
