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
// maximum number of bullets that can be created/destroyed until update is
// called (ie. per frame)
#define BULLET_STACKS_MAX_SIZE 32

typedef struct
{
	uint16_t count;
	uint16_t capacity;
	Source* sources; // the source of each bullet, ie PLAYER_ONE or TWO
	bool* disabled;	 // array of bools, true if the bullet is disabled, false
					 // otherwise
	Bullet items[0];
} BulletData;

// stack types
typedef struct
{
	uint16_t count;
	Bullet stack[BULLET_STACKS_MAX_SIZE];
} BulletCreationStack;
typedef struct
{
	uint16_t count;
	BulletHandle stack[BULLET_STACKS_MAX_SIZE];
} BulletDestructionStack;
