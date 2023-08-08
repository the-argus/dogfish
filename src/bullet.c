#include "bullet.h"
#include "bullet_internal.h"
#include "threadutils.h"
#include <stdlib.h>
#include <string.h>

// data
AABBBatchOptions bullet_data_aabb_options;
Vector3BatchOptions bullet_data_position_options;
QuaternionBatchOptions bullet_data_velocity_options;
ByteBatchOptions bullet_data_disabled_options;
static AABB universal_aabb;
static BulletData* bullet_data;
static BulletCreationStack creation_stack;
static BulletDestructionStack destruction_stack;

static Mesh bullet_mesh;
static Material bullet_material;
static Shader bullet_shader;

// helper
static bool bullet_find_disabled(uint16_t* index);
static void bullet_flush_destroy_stack();
static void bullet_flush_create_stack();

/// initialize bullet data
void bullet_init()
{
	universal_aabb = (AABB){
		.x = BULLET_PHYSICS_WIDTH,
		.y = BULLET_PHYSICS_WIDTH,
		.z = BULLET_PHYSICS_LENGTH,
	};

	bullet_data = RL_MALLOC(sizeof(BulletData) +
							(sizeof(Bullet) * BULLET_POOL_SIZE_INITIAL));
	bullet_data->count = 0;
	bullet_data->capacity = BULLET_POOL_SIZE_INITIAL;
	bullet_data->sources = NULL;
	bullet_data->disabled = RL_CALLOC(BULLET_POOL_SIZE_INITIAL, 1);
	// set disabled to true by default for all
	for (size_t i = 0; i < BULLET_POOL_SIZE_INITIAL; ++i) {
		bullet_data->disabled[i] = true;
	}

	creation_stack.count = 0;
	destruction_stack.count = 0;

	bullet_data_aabb_options = (AABBBatchOptions){
		.stride = INFINITY, // this should cause a segfault if something goes
							// wrong lol
		.first = &universal_aabb,
		.count = 1,
	};

	bullet_data_position_options = (Vector3BatchOptions){
		.count = bullet_data->count,
		.first = &bullet_data->items[0].position,
		.stride = sizeof(Bullet),
	};

	bullet_data_velocity_options = (QuaternionBatchOptions){
		.count = bullet_data->count,
		.first = &bullet_data->items[0].velocity,
		.stride = sizeof(Bullet),
	};

	bullet_data_disabled_options = (ByteBatchOptions){
		.count = bullet_data->count,
		.first = (uint8_t*)bullet_data->disabled,
		.stride = sizeof(bullet_data->disabled[0]),
	};

	// debug mesh with no normal information
	bullet_mesh = GenMeshCube(BULLET_PHYSICS_WIDTH, BULLET_PHYSICS_WIDTH,
							  BULLET_PHYSICS_LENGTH);
	bullet_material = LoadMaterialDefault();
	bullet_shader = LoadShader("assets/materials/bullet.vs",
							   "assets/materials/instanced.fs");

	// Get shader locations
	bullet_shader.locs[SHADER_LOC_MATRIX_MVP] =
		GetShaderLocation(bullet_shader, "mvp");
	// TODO: use shader locs to pass the bullet array to the gpu in DrawBullets

	// bullet_shader.locs[SHADER_LOC_MATRIX_MODEL] =
	// 	GetShaderLocationAttrib(bullet_shader, "bullet");
	bullet_material = LoadMaterialDefault();
	bullet_material.shader = bullet_shader;
	bullet_material.maps[MATERIAL_MAP_DIFFUSE].color = RED;
}

void bullet_cleanup()
{
	UnloadMesh(bullet_mesh);
	UnloadMaterial(bullet_material); // will also clean up shader
	RL_FREE(bullet_data->disabled);
	// RL_FREE(bullet_data->sources);
	RL_FREE(bullet_data);
}

/// "Create" a new bullet (actually just queues it to be created)
void bullet_create(const Vector3* restrict position,
				   const Quaternion* restrict velocity, Source source)
{
	if (creation_stack.count >=
		sizeof(creation_stack.stack) / sizeof(creation_stack.stack[0])) {
		TraceLog(LOG_WARNING,
				 "Attempted to create more than %d bullets in one frame.",
				 BULLET_STACKS_MAX_SIZE);
		return;
	}
	// push the bullet onto the creation stack
	creation_stack.stack[creation_stack.count].position = *position;
	creation_stack.stack[creation_stack.count].velocity = *velocity;
	++creation_stack.count;
}

Source bullet_get_source(BulletHandle bullet)
{
	TraceLog(LOG_FATAL, "function %s not implemented", __FUNCTION__);
	threadutils_exit(EXIT_FAILURE);
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
	// destroy bullets first to make it more likely that creation doesn't need
	// to reallocate
	bullet_flush_destroy_stack();
	bullet_flush_create_stack();
}

void bullet_draw() {}

// maybe implement this with a queue, linear search is probs expensive
static bool bullet_find_disabled(uint16_t* index)
{
	for (uint16_t i = 0; i < bullet_data->count; ++i) {
		if (bullet_data->disabled[i]) {
			*index = i;
			return true;
		}
	}
	return false;
}

static void bullet_flush_destroy_stack()
{
	// destruction is easy- we are always guaranteed to by able to perform
	// the operation of setting disabled to true
	for (uint16_t i = 0; i < destruction_stack.count; ++i) {
#ifndef NDEBUG
		if (destruction_stack.stack[i] > bullet_data->count) {
			TraceLog(LOG_ERROR,
					 "Queued bullet destruction handle %d out of range of "
					 "bullet pool with count %d",
					 destruction_stack.stack[i], bullet_data->count);
			continue;
		}
#endif
		bullet_data->disabled[destruction_stack.stack[i]] = true;
	}
	destruction_stack.count = 0;
}

static void bullet_flush_create_stack()
{
	/// creation is difficult. first try to append onto the end if there is
	/// empty space
	{
		if (creation_stack.count == 0) {
			return;
		}

		uint16_t available_spots =
			(uint16_t)fminf((float)(bullet_data->capacity - bullet_data->count),
							(float)(creation_stack.count));

		for (uint16_t i = 0; i < available_spots; ++i) {
			uint16_t index = bullet_data->count + i;
			memcpy(&bullet_data->items[index],
				   &creation_stack.stack[creation_stack.count], sizeof(Bullet));
			bullet_data->disabled[index] = false;
			--creation_stack.count;
		}

		bullet_data->count += available_spots;
		assert(bullet_data->count <= bullet_data->capacity);
	}

	// next, try to re-enable any disabled bullets
	{
		uint16_t available_index = INFINITY;
		while (true) {
			if (creation_stack.count == 0) {
				return; // done
			}
			if (!bullet_find_disabled(&available_index)) {
				break; // this method isn't going to work anymore
			}
			// found an available disabled index!!
			memcpy(&bullet_data->items[available_index],
				   &creation_stack.stack[creation_stack.count - 1],
				   sizeof(Bullet));
			bullet_data->disabled[available_index] = false;
			creation_stack.count -= 1;
		}
	}

	{
		// TODO: when implementing this, make sure to set the batch options to
		// the newly allocated structs
		TraceLog(
			LOG_FATAL,
			"Bullet data reallocation not implemented. Successfully allocated "
			"%d bullets in initial buffer",
			bullet_data->count);
		threadutils_exit(EXIT_FAILURE);
	}
}

void bullet_move_and_collide_with(
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
