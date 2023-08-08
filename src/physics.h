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

inline AABB physics_aabb_from_sphere(float radius);

/// DO NOT pass in the same pointer to any of these options
/// TODO: change the names from batch1, batch2, etc to batch_big and batch_small
inline void
physics_batch_collide(const AABBBatchOptions* restrict batch1,
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
inline void physics_batch_collide_and_move(
	const AABBBatchOptions* restrict batch1,
	const AABBBatchOptions* restrict batch2,
	const Vector3BatchOptions* restrict position_batch1,
	const Vector3BatchOptions* restrict position_batch2,
	const QuaternionBatchOptions* restrict velocity_batch1,
	const QuaternionBatchOptions* restrict velocity_batch2,
	const ByteBatchOptions* restrict disabled_batch1,
	const ByteBatchOptions* restrict disabled_batch2, CollisionHandler handler);

/// Separating Axis Theorem implementation
inline bool physics_sat(const Vector3* restrict axis, float minA, float maxA,
						float minB, float maxB, Vector3* restrict mtvAxis,
						float* restrict mtvDistance);

inline bool physics_aabb_collide(const AABB* restrict a, const AABB* restrict b,
								 const Vector3* restrict pos_a,
								 const Vector3* restrict pos_b,
								 Contact* restrict contact);

// IMPLEMENTATIONS -------------------------------------------------------------

/// Construct an AABB for a sphere of a given radius
inline AABB physics_aabb_from_sphere(float radius)
{
	uint16_t scale = (uint16_t)ceilf(radius * 2.0f);
	return (AABB){.x = scale, .y = scale, .z = scale};
}

inline void
physics_batch_collide(const AABBBatchOptions* restrict batch1,
					  const AABBBatchOptions* restrict batch2,
					  const Vector3BatchOptions* restrict position_batch1,
					  const Vector3BatchOptions* restrict position_batch2,
					  const ByteBatchOptions* restrict disabled_batch1,
					  const ByteBatchOptions* restrict disabled_batch2,
					  CollisionHandler handler)
{
	bool batch1_same_aabb = batch1->count == 1;
	bool batch2_same_aabb = batch2->count == 1;
	// you could do some pointer casts here to cause aliasing bugs.
	// please don't :(
	assert(batch1 != batch2);
	assert(position_batch1 != position_batch2);
	assert(disabled_batch1 != disabled_batch2);
	// either use the same size batches or use only one aabb definition for all
	// bodies
	assert(batch1_same_aabb || batch1->count == position_batch1->count);
	assert(batch2_same_aabb || batch2->count == position_batch2->count);
	assert(disabled_batch1 == NULL ||
		   disabled_batch1->count == position_batch1->count);
	assert(disabled_batch2 == NULL ||
		   disabled_batch2->count == position_batch2->count);
	assert(position_batch1->stride >= sizeof(Vector3));
	assert(position_batch2->stride >= sizeof(Vector3));
	assert(batch1->stride >= sizeof(AABB));
	assert(batch2->stride >= sizeof(AABB));
	// this is a check to see if the AABB stride (which we know to be aligned by
	// float) is correctly aligned. unfortunately hardcoded... I miss
	// std::alignment_of :(
	assert(batch1->stride % sizeof(float) == 0);
	assert(batch2->stride % sizeof(float) == 0);
	assert(position_batch1->stride % sizeof(float) == 0);
	assert(position_batch2->stride % sizeof(float) == 0);

	Contact contact;

	// hot hot HOT loop
	for (uint16_t outer = 0; outer < position_batch1->count; ++outer) {
		// may be necessary to skip this body
		if (disabled_batch1 != NULL) {
			if (*(disabled_batch1->first +
				  ((ptrdiff_t)outer * disabled_batch1->stride)) != 0) {
				continue;
			}
		}

		AABB* outer_aabb = NULL;

		// TODO: this branch might be slowing things down. do profiling in
		// release mode
		if (batch1_same_aabb) {
			outer_aabb = batch1->first;
		} else {
			outer_aabb = (AABB*)((uint8_t*)batch1->first +
								 ((ptrdiff_t)outer * batch1->stride));
		}

		Vector3* outer_position =
			(Vector3*)((uint8_t*)position_batch1->first +
					   ((ptrdiff_t)outer * position_batch1->stride));

		for (uint16_t inner = 0; inner < batch2->count; ++inner) {
			// may be necessary to skip this body
			if (disabled_batch2 != NULL) {
				if (*(disabled_batch2->first +
					  ((ptrdiff_t)inner * disabled_batch2->stride)) != 0) {
					continue;
				}
			}
			AABB* inner_aabb = NULL;

			// TODO: this branch might be slowing things down. do profiling in
			// release mode
			if (batch2_same_aabb) {
				inner_aabb = batch2->first;
			} else {
				inner_aabb = (AABB*)((uint8_t*)batch2->first +
									 ((ptrdiff_t)inner * batch2->stride));
			}

			Vector3* inner_position =
				(Vector3*)((uint8_t*)position_batch2->first +
						   ((ptrdiff_t)inner * position_batch2->stride));

			if (physics_aabb_collide(outer_aabb, inner_aabb, outer_position,
									 inner_position, &contact)) {
				switch (handler(outer, inner, &contact)) {
				case CANCEL:
					return;
				case CONTINUE:
					break;
				default:
					TraceLog(LOG_WARNING, "Invalid return code, corruption?");
					break;
				}
			}
		}
	}
}

inline void physics_batch_collide_and_move(
	const AABBBatchOptions* restrict batch1,
	const AABBBatchOptions* restrict batch2,
	const Vector3BatchOptions* restrict position_batch1,
	const Vector3BatchOptions* restrict position_batch2,
	const QuaternionBatchOptions* restrict velocity_batch1,
	const QuaternionBatchOptions* restrict velocity_batch2,
	const ByteBatchOptions* restrict disabled_batch1,
	const ByteBatchOptions* restrict disabled_batch2, CollisionHandler handler)
{
	bool batch1_same_aabb = batch1->count == 1;
	bool batch2_same_aabb = batch2->count == 1;
	// you could do some pointer casts here to cause aliasing bugs.
	// please don't :(
	assert(batch1 != batch2);
	assert(position_batch1 != position_batch2);
	assert(velocity_batch1 != velocity_batch2);
	assert(disabled_batch1 != disabled_batch2);
	// either use the same size batches or use only one aabb definition for all
	// bodies
	assert(batch1_same_aabb || batch1->count == position_batch1->count);
	assert(batch2_same_aabb || batch2->count == position_batch2->count);
	assert(batch1_same_aabb || batch1->count == velocity_batch1->count);
	assert(batch2_same_aabb || batch2->count == velocity_batch2->count);
	assert(disabled_batch1 == NULL ||
		   disabled_batch1->count == position_batch1->count);
	assert(disabled_batch2 == NULL ||
		   disabled_batch2->count == position_batch2->count);
	assert(position_batch1->stride >= sizeof(Vector3));
	assert(position_batch2->stride >= sizeof(Vector3));
	assert(velocity_batch1->stride >= sizeof(Vector3));
	assert(velocity_batch2->stride >= sizeof(Vector3));
	assert(batch1->stride >= sizeof(AABB));
	assert(batch2->stride >= sizeof(AABB));
	// this is a check to see if the AABB stride (which we know to be aligned by
	// float) is correctly aligned. unfortunately hardcoded... I miss
	// std::alignment_of :(
	assert(batch1->stride % sizeof(float) == 0);
	assert(batch2->stride % sizeof(float) == 0);
	assert(position_batch1->stride % sizeof(float) == 0);
	assert(position_batch2->stride % sizeof(float) == 0);
	assert(velocity_batch1->stride % sizeof(float) == 0);
	assert(velocity_batch2->stride % sizeof(float) == 0);

	Contact contact;

	// hot hot HOT loop
	for (uint16_t outer = 0; outer < batch1->count; ++outer) {
		// may be necessary to skip this body
		if (disabled_batch1 != NULL) {
			if (*(disabled_batch1->first +
				  ((ptrdiff_t)outer * disabled_batch1->stride)) != 0) {
				continue;
			}
		}
		AABB* outer_aabb = NULL;

		// TODO: this branch might be slowing things down. do profiling in
		// release mode
		if (batch1_same_aabb) {
			outer_aabb = batch1->first;
		} else {
			outer_aabb = (AABB*)((uint8_t*)batch1->first +
								 ((ptrdiff_t)outer * batch1->stride));
		}

		Vector3* outer_position =
			(Vector3*)((uint8_t*)position_batch1->first +
					   ((ptrdiff_t)outer * position_batch1->stride));

		Quaternion* outer_velocity =
			(Quaternion*)((uint8_t*)velocity_batch1->first +
						  ((ptrdiff_t)outer * velocity_batch1->stride));

		// TODO: profile this math

		Vector3 outer_real_velocity = Vector3RotateByQuaternion(
			(Vector3){0.0f, 1.0f, 0.0f}, *outer_velocity);
		outer_real_velocity = Vector3Scale(outer_real_velocity,
										   QuaternionLength(*outer_velocity));

		*outer_position = Vector3Add(*outer_position, outer_real_velocity);

		for (uint16_t inner = 0; inner < batch2->count; ++inner) {
			// may be necessary to skip this body
			if (disabled_batch2 != NULL) {
				if (*(disabled_batch2->first +
					  ((ptrdiff_t)inner * disabled_batch2->stride)) != 0) {
					continue;
				}
			}
			AABB* inner_aabb = NULL;

			// TODO: this branch might be slowing things down. do profiling in
			// release mode
			if (batch2_same_aabb) {
				inner_aabb = batch2->first;
			} else {
				inner_aabb = (AABB*)((uint8_t*)batch2->first +
									 ((ptrdiff_t)inner * batch2->stride));
			}

			Vector3* inner_position =
				(Vector3*)((uint8_t*)position_batch2->first +
						   ((ptrdiff_t)inner * position_batch2->stride));

			Quaternion* inner_velocity =
				(Quaternion*)((uint8_t*)velocity_batch2->first +
							  ((ptrdiff_t)inner * velocity_batch2->stride));

			Vector3 inner_real_velocity = Vector3RotateByQuaternion(
				(Vector3){0.0f, 1.0f, 0.0f}, *inner_velocity);
			inner_real_velocity = Vector3Scale(
				inner_real_velocity, QuaternionLength(*inner_velocity));

			*inner_position = Vector3Add(*inner_position, inner_real_velocity);

			if (physics_aabb_collide(outer_aabb, inner_aabb, outer_position,
									 inner_position, &contact)) {
				switch (handler(outer, inner, &contact)) {
				case CANCEL:
					return;
				case CONTINUE:
					break;
				default:
					TraceLog(LOG_WARNING, "Invalid return code, corruption?");
					break;
				}
			}
		}
	}
}

// stolen from
// https://gamedev.stackexchange.com/questions/129446/how-can-i-calculate-the-penetration-depth-between-two-colliding-3d-aabbs

inline bool physics_aabb_collide(const AABB* restrict a, const AABB* restrict b,
								 const Vector3* restrict pos_a,
								 const Vector3* restrict pos_b,
								 Contact* restrict contact)
{
	assert(a != b);
	assert(pos_a != pos_b);
	// Minimum Translation Vector
	// ==========================
	// Set current minimum distance (max float value so next value is always
	// less)
	float mtvDistance = INFINITY;
	// Axis along which to travel with the minimum distance
	Vector3 mtvAxis = {0.0f, 0.0f, 0.0f};

	// Axes of potential separation
	// ============================
	// - Each shape must be projected on these axes to test for intersection:
	//
	// (1, 0, 0)                    A0 (= B0) [X Axis]
	// (0, 1, 0)                    A1 (= B1) [Y Axis]
	// (0, 0, 1)                    A1 (= B2) [Z Axis]
	const static Vector3 xaxis = {1.0f, 0.0f, 0.0f};
	const static Vector3 yaxis = {0.0f, 1.0f, 0.0f};
	const static Vector3 zaxis = {0.0f, 0.0f, 1.0f};

	// [X Axis]
	if (!physics_sat(&xaxis, pos_a->x - a->x, pos_a->x + a->x, pos_b->x - b->x,
					 pos_b->x - b->x, &mtvAxis, &mtvDistance)) {
		return false;
	}

	// [Y Axis]
	if (!physics_sat(&yaxis, pos_a->y - a->y, pos_a->y + a->y, pos_b->y - b->y,
					 pos_b->y - b->y, &mtvAxis, &mtvDistance)) {
		return false;
	}

	// [Z Axis]
	if (!physics_sat(&zaxis, pos_a->z - a->z, pos_a->z + a->z, pos_b->z - b->z,
					 pos_b->z - b->z, &mtvAxis, &mtvDistance)) {
		return false;
	}

	// Calculate Minimum Translation Vector (MTV) [normal * penetration]
	contact->collision = Vector3Normalize(mtvAxis);

	// Multiply the penetration depth by itself plus a small increment
	// When the penetration is resolved using the MTV, it will no longer
	// intersect
	contact->depth = sqrtf(mtvDistance) * 1.001f;

	return true;
}

inline bool physics_sat(const Vector3* restrict axis, float minA, float maxA,
						float minB, float maxB, Vector3* restrict mtvAxis,
						float* restrict mtvDistance)
{
	assert(axis != mtvAxis);
	assert((void*)mtvDistance != (void*)mtvAxis);
	// Separating Axis Theorem
	// =======================
	// - Two convex shapes only overlap if they overlap on all axes of
	// separation
	// - In order to create accurate responses we need to find the collision
	// vector (Minimum Translation Vector)
	// - The collision vector is made from a vector and a scalar,
	//   - The vector value is the axis associated with the smallest penetration
	//   - The scalar value is the smallest penetration value
	// - Find if the two boxes intersect along a single axis
	// - Compute the intersection interval for that axis
	// - Keep the smallest intersection/penetration value
	float axisLengthSquared = Vector3DotProduct(*axis, *axis);

	// If the axis is degenerate then ignore
	if (axisLengthSquared < 1.0e-8f) {
		return true;
	}

	// Calculate the two possible overlap ranges
	// Either we overlap on the left or the right sides
	float d0 = (maxB - minA); // 'Left' side
	float d1 = (maxA - minB); // 'Right' side

	// Intervals do not overlap, so no intersection
	if (d0 <= 0.0f || d1 <= 0.0f) {
		return false;
	}

	// Find out if we overlap on the 'right' or 'left' of the object.
	float overlap = (d0 < d1) ? d0 : -d1;

	// The mtd vector for that axis
	Vector3 sep = Vector3Scale(*axis, (overlap / axisLengthSquared));

	// The mtd vector length squared
	float sepLengthSquared = Vector3DotProduct(sep, sep);

	// If that vector is smaller than our computed Minimum Translation Distance
	// use that vector as our current MTV distance
	if (sepLengthSquared < *mtvDistance) {
		*mtvDistance = sepLengthSquared;
		*mtvAxis = sep;
	}

	return true;
}
