#include "bullet.h"
#include "bullet_internal.h"
#include "bullet_render.h"
#include "quicksort.h"
#include "threadutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: make thread safe- when create or destroy stacks are being modified,
// don't read from bullet data. (those are the only writers to the data so
// otherwise it should be safe)

// data
static AABBBatchOptions bullet_data_aabb_options;
static Vector3BatchOptions bullet_data_position_options;
static QuaternionBatchOptions bullet_data_direction_options;
static FloatBatchOptions bullet_data_speed_options;
static AABB universal_aabb;
static float universal_speed;
static BulletData* bullet_data;
static BulletCreationStack creation_stack;
static BulletDestructionStack destruction_stack;

static Mesh bullet_mesh;
static Material bullet_material;
static Shader bullet_shader;

#ifndef NDEBUG
static uint8_t bullet_times_moved_on_frame = 0;
#endif

// helper
static void bullet_flush_destroy_stack();
static void bullet_flush_create_stack();
static void bullet_set_batch_options_for_dynamically_allocated_memory();
static void bullet_despawn_old();
static void bullet_increase_allocation();

/// initialize bullet data
void bullet_init()
{
	universal_speed = 1;
	universal_aabb = (AABB){
		.x = BULLET_PHYSICS_WIDTH,
		.y = BULLET_PHYSICS_WIDTH,
		.z = BULLET_PHYSICS_LENGTH,
	};

	bullet_data = RL_MALLOC(sizeof(BulletData) +
							(sizeof(Bullet) * BULLET_POOL_SIZE_INITIAL));
	CHECKMEM(bullet_data);
	bullet_data->count = 0;
	bullet_data->capacity = BULLET_POOL_SIZE_INITIAL;
	bullet_data->sources = RL_MALLOC(BULLET_POOL_SIZE_INITIAL * sizeof(Source));
	bullet_data->create_times =
		RL_MALLOC(BULLET_POOL_SIZE_INITIAL * sizeof(double));
	CHECKMEM(bullet_data->disabled);
	// set disabled to true by default for all
	for (size_t i = 0; i < BULLET_POOL_SIZE_INITIAL; ++i) {
#ifndef NDEBUG
		bullet_data->sources[i] = PLAYER_NULL;
		bullet_data->create_times[i] = INFINITY;
#endif
	}

	creation_stack.count = 0;
	destruction_stack.count = 0;

	bullet_data_aabb_options = (AABBBatchOptions){
		.stride = NOSTRIDE, // arbitrary large prime number
		.first = &universal_aabb,
		.count = 0,
	};

	bullet_data_speed_options = (FloatBatchOptions){
		.stride = NOSTRIDE,
		.first = &universal_speed,
		.count = 0,
	};

	bullet_set_batch_options_for_dynamically_allocated_memory();

	// debug mesh with no normal information
	bullet_mesh = GenMeshCube(BULLET_PHYSICS_WIDTH, BULLET_PHYSICS_WIDTH,
							  BULLET_PHYSICS_LENGTH);
	bullet_material = LoadMaterialDefault();
	bullet_shader = LoadShader("assets/materials/bullet.vert",
							   "assets/materials/instanced.frag");

	// Get shader locations
	bullet_shader.locs[SHADER_LOC_MATRIX_MVP] =
		GetShaderLocation(bullet_shader, "mvp");
	bullet_shader.locs[BULLET_SHADER_LOC_POSITION] =
		GetShaderLocationAttrib(bullet_shader, "bulletPosition");
	bullet_shader.locs[BULLET_SHADER_LOC_VELOCITY] =
		GetShaderLocationAttrib(bullet_shader, "bulletVelocity");

	bullet_material = LoadMaterialDefault();
	bullet_material.shader = bullet_shader;
	bullet_material.maps[MATERIAL_MAP_DIFFUSE].color = RED;
}

void bullet_cleanup()
{
	UnloadMesh(bullet_mesh);
	UnloadMaterial(bullet_material); // will also clean up shader
	RL_FREE(bullet_data->sources);
	RL_FREE(bullet_data->create_times);
	RL_FREE(bullet_data);
}

static void bullet_set_batch_options_for_dynamically_allocated_memory()
{
	bullet_data_position_options = (Vector3BatchOptions){
		.count = bullet_data->count,
		.first = &bullet_data->items[0].position,
		.stride = sizeof(Bullet),
	};

	bullet_data_direction_options = (QuaternionBatchOptions){
		.count = bullet_data->count,
		.first = &bullet_data->items[0].direction,
		.stride = sizeof(Bullet),
	};
}

/// "Create" a new bullet (actually just queues it to be created)
void bullet_create(const BulletCreateOptions* options)
{
	assert(QuaternionEquals(QuaternionNormalize(options->bullet.direction),
							options->bullet.direction));
	if (creation_stack.count >=
		sizeof(creation_stack.stack) / sizeof(creation_stack.stack[0])) {
		TraceLog(LOG_WARNING,
				 "Attempted to create more than %d bullets in one frame.",
				 BULLET_STACKS_MAX_SIZE);
		return;
	}
	// push the bullet onto the creation stack
	creation_stack.stack[creation_stack.count] = *options;
	++creation_stack.count;
}

Source bullet_get_source(BulletHandle bullet)
{
	// this is the only time we read from sources
	assert(bullet_data->sources[bullet.raw] != PLAYER_NULL);
	return bullet_data->sources[bullet.raw];
}

void bullet_destroy(BulletHandle bullet)
{
	if (destruction_stack.count >=
		sizeof(destruction_stack.stack) / sizeof(destruction_stack.stack[0])) {
		TraceLog(LOG_WARNING,
				 "Attempted to create more than %d bullets in one frame.",
				 BULLET_STACKS_MAX_SIZE);
		return;
	}
	destruction_stack.stack[destruction_stack.count] = bullet;
	++destruction_stack.count;
}

void bullet_update()
{
#ifndef NDEBUG
	// reset count of how many times bullet_move_and_collide_with is called
	bullet_times_moved_on_frame = 0;
#endif
	// destroy bullets first to make it more likely that creation doesn't need
	// to reallocate
	// first flush is for bullets other modules have queued for destruction
	// since last update
	bullet_flush_destroy_stack();
	bullet_despawn_old();
	// now flush the despawned bullets
	bullet_flush_destroy_stack();
	bullet_flush_create_stack();

	// options that we pass to physics in the next frame must respect any newly
	// created bullets
	bullet_data_position_options.count = bullet_data->count;
	bullet_data_direction_options.count = bullet_data->count;
	// bullet_data_speed_options.count = 1;
	// bullet_data_aabb_options.count = 1;

	// NOTE: if (bullet_data->count > 0)... but eh branchless is probably better
	bullet_data_position_options.first = &bullet_data->items[0].position;
	bullet_data_direction_options.first = &bullet_data->items[0].direction;
	// bullet_data_speed_options.first = &universal_speed;
	// bullet_data_aabb_options.first = &universal_aabb;
}

void bullet_draw()
{
	DrawBullets(&bullet_mesh, &bullet_material, bullet_data->items,
				bullet_data->count);
}

static void bullet_flush_destroy_stack()
{
	if (destruction_stack.count == 0) {
		return;
	}
	// sort destroy operations by index
	quicksort_inplace_uint16(&destruction_stack.stack[0].raw,
							 destruction_stack.count, sizeof(BulletHandle));

	// traverse backwards because quicksort sorts low to high, but we want
	// to remove the items at the end first
	for (int i = destruction_stack.count - 1; i >= 0; --i) {
		const uint16_t index = destruction_stack.stack[i].raw;
#ifndef NDEBUG
		if (index > bullet_data->count) {
			TraceLog(LOG_FATAL,
					 "Queued bullet destruction handle %d out of range of "
					 "bullet pool with count %d",
					 index, bullet_data->count);
			threadutils_exit(EXIT_FAILURE);
		}
#endif
		// overwrite the destroyed bullet with the bullet at the end of the
		// bullet list
		bullet_data->items[index] = bullet_data->items[bullet_data->count - 1];
		bullet_data->sources[index] =
			bullet_data->sources[bullet_data->count - 1];
		bullet_data->create_times[index] =
			bullet_data->create_times[bullet_data->count - 1];
		// no longer incude the now-copied bullet
		--bullet_data->count;
	}
	destruction_stack.count = 0;
}

static void bullet_flush_create_stack()
{
	if (creation_stack.count == 0) {
		return;
	}

	// find how many allocated but unused spots there are at the end
	uint16_t available_spots = bullet_data->capacity - bullet_data->count;

	// maybe reallocate
	static const uint8_t max_reallocs = 1;
	for (uint8_t i = 0; i < max_reallocs + 1; ++i) {
		if (creation_stack.count > available_spots) {
			// NOTE: thanks to despawning bullets, this code almost never
			// runs. given a known player count reallocation could be
			// removed altogether due to the theoretical limits of how many
			// bullets the players can shoot
			bullet_increase_allocation();
			available_spots = bullet_data->capacity - bullet_data->count;
		} else {
			break;
		}
	}

	if (creation_stack.count > available_spots) {
		TraceLog(
			LOG_FATAL,
			"Reallocation of bullet buffer failed given %d max reallocations",
			max_reallocs);
		threadutils_exit(EXIT_FAILURE);
	}

	// space should be available at this point
	// fill newly made space with the bullets
	for (uint16_t i = 0; i < creation_stack.count; ++i) {
		assert(creation_stack.count > 0);
		assert(bullet_data->count <= bullet_data->capacity);

		// create a bullet at the end of the bullet data
		const BulletCreateOptions* create_options = &creation_stack.stack[i];
		const uint16_t index = bullet_data->count;
		// make sure bullet has a valid normalized direction
		assert(QuaternionEquals(
			QuaternionNormalize(create_options->bullet.direction),
			create_options->bullet.direction));
		bullet_data->items[index] = create_options->bullet;
		bullet_data->sources[index] = create_options->source;
		bullet_data->create_times[index] = GetTime();
		++bullet_data->count;
	}
	creation_stack.count = 0;
	assert(bullet_data->count <= bullet_data->capacity);
}

static void bullet_increase_allocation()
{
	size_t new_size =
		(long)bullet_data->capacity * BULLET_ALLOCATION_SCALE_FACTOR;

	if (new_size > BULLET_POOL_MAX_POSSIBLE_COUNT) {
		TraceLog(LOG_FATAL,
				 "More bullets fired than an unsigned 16 bit integer can "
				 "describe. Increase the integer size used for array "
				 "indices if you want more bullets");
		threadutils_exit(EXIT_FAILURE);
	}

	bullet_data = RL_REALLOC(bullet_data,
							 sizeof(BulletData) + (sizeof(Bullet) * new_size));

	bullet_data->sources =
		RL_REALLOC(bullet_data->sources, new_size * sizeof(Source));
	bullet_data->create_times =
		RL_REALLOC(bullet_data->create_times, new_size * sizeof(double));
#ifndef NDEBUG
	// set all the new memory to defaults
	for (size_t i = bullet_data->capacity; i < new_size; ++i) {
		bullet_data->sources[i] = PLAYER_NULL;
		bullet_data->create_times[i] = INFINITY;
	}
#endif
	bullet_data->capacity = new_size;

	bullet_set_batch_options_for_dynamically_allocated_memory();
}

void bullet_move_and_collide_with(
	const AABBBatchOptions* restrict other_aabb,
	const Vector3BatchOptions* restrict other_position,
	const QuaternionBatchOptions* restrict other_direction,
	const FloatBatchOptions* restrict other_speed, CollisionHandler handler)
{
#ifndef NDEBUG
	++bullet_times_moved_on_frame;
	// just dont call this function more than once per frame
	assert(bullet_times_moved_on_frame == 1);
#endif
	assert((void*)other_position != (void*)other_direction);
	assert((void*)other_aabb != (void*)other_speed);
	assert((void*)other_aabb != (void*)other_position);
	// TODO: maybe try swapping the bullet data to be the second batch so it's
	// in the inner loop. has performance implications.
	// TODO: consider using point-to-aabb collisions for bullets, may be cheaper
	physics_batch_collide_and_move(
		&bullet_data_aabb_options, other_aabb, &bullet_data_position_options,
		other_position, &bullet_data_direction_options, other_direction,
		&bullet_data_speed_options, other_speed, handler);
}

static void bullet_despawn_old()
{
	static size_t times_called;
	++times_called;

	if (times_called < CALLS_PER_DESPAWN) {
		return;
	}

	const double current = GetTime();
	for (size_t i = 0; i < bullet_data->count; ++i) {
		// times initialized to INFINITY, make sure its not that
		assert(bullet_data->create_times[i] < current);
		if (current - bullet_data->create_times[i] >= DESPAWN_TIME_SECONDS) {
			bullet_destroy((BulletHandle){.raw = i});
		}
	}

	times_called = 0;
}
