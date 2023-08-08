#pragma once
///
/// Provides a common interface for defining AABBs and callbacks when two AABBs
/// are near each other.
///
#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stddef.h>
#include <stdint.h>

// The extents of the AABB from its Vector3 position. e.g. x = 0.5 means that
// the box is 1 unit wide on X, extending 0.5 units in each direction.
typedef Vector3 AABB;

/// A standard way of allocating batch of AABBs. Not directly required by the
/// physics functions (stride is available if you want to use something else)
typedef struct
{
	uint16_t count;
	uint16_t capacity;
	AABB contents[0];
} AABBBatch;

typedef struct
{
	uint16_t count;
	uint16_t capacity;
	Vector3 contents[0];
} Vector3Batch;

typedef struct
{
	/// Arbitrary array of AABBs. May be an AABBBatch
	AABB* first;
	/// Number of AABBs present in the array
	uint16_t count;
	/// Distance in bytes between each AABB
	uint8_t stride;
} AABBBatchOptions;

/// Options describing a pool of positions, tied by index to AABBs.
typedef struct
{
	Vector3* first;
	uint16_t count;
	uint8_t stride;
} Vector3BatchOptions;

/// options describing a pool of bytes (often bools)
typedef struct
{
	uint8_t* first;
	uint16_t count;
	uint8_t stride;
} ByteBatchOptions;

/// options describing a pool of quaternions
typedef struct
{
	Quaternion* first;
	uint16_t count;
	uint8_t stride;
} QuaternionBatchOptions;

typedef struct
{
	Vector3 collision;
	float depth;
} Contact;

typedef enum : uint8_t
{
	CANCEL,
	CONTINUE
} CollisionHandlerReturnCode;

typedef CollisionHandlerReturnCode (*CollisionHandler)(uint16_t index_batch1,
													   uint16_t index_batch2,
													   Contact* contact);

AABB physics_aabb_from_sphere(float radius);

/// DO NOT pass in the same pointer to any of these options
/// TODO: change the names from batch1, batch2, etc to batch_big and batch_small
void physics_batch_collide(const AABBBatchOptions* restrict batch1,
						   const AABBBatchOptions* restrict batch2,
						   const Vector3BatchOptions* restrict position_batch1,
						   const Vector3BatchOptions* restrict position_batch2,
						   const ByteBatchOptions* restrict disabled_batch1,
						   const ByteBatchOptions* restrict disabled_batch2,
						   CollisionHandler handler);

/// Provides a simple optimization over physics_batch_collide where you also
/// move the positions by their respective velocities during the loop. This
/// pattern was decided because things like bullets and particles tend to have
/// a single constant velocity which need to be added to each frame.
/// DO NOT pass in the same pointer to any of these options.
void physics_batch_collide_and_move(
	const AABBBatchOptions* restrict batch1,
	const AABBBatchOptions* restrict batch2,
	const Vector3BatchOptions* restrict position_batch1,
	const Vector3BatchOptions* restrict position_batch2,
	const QuaternionBatchOptions* restrict velocity_batch1,
	const QuaternionBatchOptions* restrict velocity_batch2,
	const ByteBatchOptions* restrict disabled_batch1,
	const ByteBatchOptions* restrict disabled_batch2, CollisionHandler handler);

/// Separating Axis Theorem implementation
bool physics_sat(const Vector3* restrict axis, float minA, float maxA,
				 float minB, float maxB, Vector3* restrict mtvAxis,
				 float* restrict mtvDistance);

bool physics_aabb_collide(const AABB* restrict a, const AABB* restrict b,
						  const Vector3* restrict pos_a,
						  const Vector3* restrict pos_b,
						  Contact* restrict contact);
