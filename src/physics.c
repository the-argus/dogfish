#include "physics.h"

/// Construct an AABB for a sphere of a given radius
AABB physics_aabb_from_sphere(float radius)
{
	uint16_t scale = (uint16_t)ceilf(radius * 2.0f);
	return (AABB){.x = scale, .y = scale, .z = scale};
}

void physics_batch_collide(const AABBBatchOptions* restrict batch1,
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
	assert(batch1_same_aabb || batch1->stride >= sizeof(AABB));
	assert(batch2_same_aabb || batch2->stride >= sizeof(AABB));
	// this is a check to see if the AABB stride (which we know to be aligned by
	// float) is correctly aligned. unfortunately hardcoded... I miss
	// std::alignment_of :(
	assert(batch1_same_aabb || batch1->stride % sizeof(float) == 0);
	assert(batch2_same_aabb || batch2->stride % sizeof(float) == 0);
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

static void move(Vector3* restrict position, uint16_t index,
				 const QuaternionBatchOptions* restrict direction_batch,
				 const FloatBatchOptions* restrict speed_batch, bool same_speed)
{
	Quaternion* dir =
		(Quaternion*)((uint8_t*)direction_batch->first +
					  ((ptrdiff_t)index * direction_batch->stride));

	float speed = 0;
	if (same_speed == true) {
		speed = *speed_batch->first;
	} else {
		speed = *(float*)((uint8_t*)speed_batch->first +
						  ((ptrdiff_t)index * speed_batch->stride));
	}

	assert(QuaternionEquals(QuaternionNormalize(*dir), *dir));

	// TODO: profile this math
	// *position = Vector3Add(
	// 	*position, Vector3RotateByQuaternion((Vector3){0, 0, speed}, *dir));

	Vector3 move_by = {speed, 0, 0};
	// move_by =
	// 	Vector3RotateByAxisAngle(move_by, (Vector3){0, 1, 0}, camera->angle.x);
    move_by = Vector3RotateByQuaternion(move_by, *dir);

	// right vector is the axis
	// Vector3 axis = Vector3CrossProduct((Vector3){0, 1, 0}, move_by);
	// move_by = Vector3RotateByAxisAngle(move_by, axis, camera->angle.y);
    *position = Vector3Add(*position, move_by);
}

void physics_batch_collide_and_move(
	const AABBBatchOptions* restrict batch1,
	const AABBBatchOptions* restrict batch2,
	const Vector3BatchOptions* restrict position_batch1,
	const Vector3BatchOptions* restrict position_batch2,
	const QuaternionBatchOptions* restrict direction_batch1,
	const QuaternionBatchOptions* restrict direction_batch2,
	const FloatBatchOptions* restrict speed_batch1,
	const FloatBatchOptions* restrict speed_batch2,
	const ByteBatchOptions* restrict disabled_batch1,
	const ByteBatchOptions* restrict disabled_batch2, CollisionHandler handler)
{
	bool batch1_same_aabb = batch1->count == 1;
	bool batch2_same_aabb = batch2->count == 1;
	bool batch1_same_speed = speed_batch1->count == 1;
	bool batch2_same_speed = speed_batch2->count == 1;
	// you could do some pointer casts here to cause aliasing bugs.
	// please don't :(
	assert(batch1 != batch2);
	assert(position_batch1 != position_batch2);
	assert(speed_batch1 != speed_batch2);
	assert(direction_batch1 != direction_batch2);
	assert(disabled_batch1 != disabled_batch2);
	// either use the same size batches or use only one aabb definition for all
	// bodies
	assert(batch1_same_aabb || batch1->count == position_batch1->count);
	assert(batch2_same_aabb || batch2->count == position_batch2->count);
	assert(batch1_same_aabb || batch1->count == direction_batch1->count);
	assert(batch2_same_aabb || batch2->count == direction_batch2->count);
	assert(batch1_same_speed || batch1->count == speed_batch1->count);
	assert(batch2_same_speed || batch2->count == speed_batch2->count);
	assert(disabled_batch1 == NULL ||
		   disabled_batch1->count == position_batch1->count);
	assert(disabled_batch2 == NULL ||
		   disabled_batch2->count == position_batch2->count);
	assert(position_batch1->stride >= sizeof(Vector3));
	assert(position_batch2->stride >= sizeof(Vector3));
	assert(batch1_same_speed || speed_batch1->stride >= sizeof(float));
	assert(batch2_same_speed || speed_batch2->stride >= sizeof(float));
	assert(batch1_same_aabb || batch1->stride >= sizeof(AABB));
	assert(batch2_same_aabb || batch2->stride >= sizeof(AABB));
	// this is a check to see if the AABB stride (which we know to be aligned by
	// float) is correctly aligned. unfortunately hardcoded... I miss
	// std::alignment_of :(
	assert(batch1_same_aabb || batch1->stride % sizeof(float) == 0);
	assert(batch2_same_aabb || batch2->stride % sizeof(float) == 0);
	assert(position_batch1->stride % sizeof(float) == 0);
	assert(position_batch2->stride % sizeof(float) == 0);
	assert(batch1_same_speed || speed_batch1->stride % sizeof(float) == 0);
	assert(batch2_same_speed || speed_batch2->stride % sizeof(float) == 0);
	assert(direction_batch1->stride % sizeof(float) == 0);
	assert(direction_batch2->stride % sizeof(float) == 0);
	assert(disabled_batch1 == NULL || disabled_batch1->stride != 0);
	assert(disabled_batch2 == NULL || disabled_batch2->stride != 0);

	Contact contact;

	// it's possible for the outer loop to never fire because there are no items
	// in it. in that case the inner loop will also not fire, even if there are
	// items. deal with that here
	if (position_batch1->count == 0 && position_batch2->count != 0) {
		for (uint16_t i = 0; i < position_batch2->count; ++i) {
			Vector3* pos = (Vector3*)((uint8_t*)position_batch2->first +
									  ((ptrdiff_t)i * position_batch2->stride));
			move(pos, i, direction_batch2, speed_batch2, batch2_same_speed);
		}
	}

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

		move(outer_position, outer, direction_batch1, speed_batch1,
			 batch1_same_speed);

		for (uint16_t inner = 0; inner < position_batch2->count; ++inner) {
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

			// also move all the inner bodies on the first run through
			if (outer == 0) {
				move(inner_position, inner, direction_batch2, speed_batch2,
					 batch2_same_speed);
			}

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

bool physics_aabb_collide(const AABB* restrict a, const AABB* restrict b,
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

bool physics_sat(const Vector3* restrict axis, float minA, float maxA,
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
