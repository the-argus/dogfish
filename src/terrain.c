#include "terrain_internal.h"
#include "constants/general.h"
#include "mesher.h"
#include "rlights.h"
#include "terrain.h"
#include "terrain_voxel_data.h"
#include "threadutils.h"
#include <math.h>
#include <pthread.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

// TODO: make the wording in this module more consistent/clear: what is a chunk
// vs. voxels

// TODO: remove responsibility for terrain_data stuff from this module.
// allocation and deallocation should be separate, as well as the process of
// assuring that on a given frame you clean up all the available_indices

typedef struct
{
	bool dirty;
	Vector3 prev_coords;
	Vector3 coords;
} PlayerPosition;

typedef struct
{
	bool working;
	pthread_t pthread;
} LoaderThread;

typedef struct
{
	ChunkCoords chunk_coords;
	uint8_t thread_index;
	Chunk* out_chunk;
} GenerationArgs;

#define NUM_THREADS 8

static TerrainData* terrain_data;
static Material terrain_mat;
static PlayerPosition* player_positions;
static RenderTexture texture_atlas;
static LoaderThread* threads;
static IntermediateVoxelData* threaded_voxel_data;
static GenerationArgs* generation_jobs;

static void terrain_generate_mesh_for_chunk(GenerationArgs* args);

static void terrain_update_chunks();

static void* terrain_generate_mesh_for_chunk_thread_wrapper(void* args);

static void terrain_start_generation_job(ChunkCoords chunk_coords,
										 uint8_t thread_index,
										 Chunk* out_chunk);

void terrain_draw()
{
	for (size_t i = 0; i < terrain_data->count; ++i) {
		const ChunkCoords* pos = &terrain_data->chunks[i].position;
		DrawMesh(terrain_data->chunks[i].mesh, terrain_mat,
				 MatrixTranslate((float)pos->x * (1.0f / CHUNK_SIZE), 0,
								 (float)pos->z * (1.0f / CHUNK_SIZE)));
	}
}

void terrain_load()
{
	init_noise();
	// permanent
	size_t num_meshes = (size_t)RENDER_DISTANCE * RENDER_DISTANCE * NUM_PLANES;
	terrain_data = RL_MALLOC(sizeof(TerrainData) +
							 (num_meshes * sizeof(terrain_data->chunks[0])));
	for (size_t i = 0; i < num_meshes; ++i) {
		terrain_data->chunks[i].loaders = 0;
	}
	terrain_data->capacity = num_meshes;
	terrain_data->count = 0;
	terrain_data->available_indices = RL_MALLOC(
		sizeof(AvailableIndicesStack) +
		(sizeof(terrain_data->available_indices->indices[0]) * num_meshes));
	terrain_data->available_indices->count = 0;
	terrain_data->available_indices->capacity = num_meshes;
	player_positions = RL_CALLOC(NUM_PLANES, sizeof(PlayerPosition));

	// heap allocated for debugging, could be statically allocated
	threads = RL_CALLOC(NUM_THREADS, sizeof(LoaderThread));
	threaded_voxel_data = RL_CALLOC(NUM_THREADS, sizeof(IntermediateVoxelData));
	generation_jobs = RL_CALLOC(NUM_THREADS, sizeof(GenerationArgs));

	// set up texture atlas and UV rects
	// first draw textures into atlas
	static const float texsize = 32;
	texture_atlas = LoadRenderTexture((int)texsize, (int)texsize);
	BeginTextureMode(texture_atlas);
	// currently the atlas is just one gray square. more textures can be added
	// in the future
	ClearBackground(GRAY);
	EndTextureMode();
	// store that texture into material
	terrain_mat = LoadMaterialDefault();
	terrain_mat.maps[0].color = WHITE;
	terrain_mat.maps[0].texture = texture_atlas.texture;

	Shader shader = LoadShader("assets/materials/basic_lit.vert",
							   "assets/materials/basic_lit.frag");
	shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

	int ambientLoc = GetShaderLocation(shader, "ambient");
	float val[4] = {0.1f, 0.1f, 0.1f, 1.0f};
	SetShaderValue(shader, ambientLoc, val, SHADER_UNIFORM_VEC4);

	Light lights[MAX_LIGHTS] = {0};
	lights[0] = CreateLight(LIGHT_DIRECTIONAL, Vector3Zero(),
							(Vector3){-2, -4, -3}, WHITE, shader);
	lights[1] = CreateLight(LIGHT_DIRECTIONAL, Vector3Zero(),
							(Vector3){2, 2, 5}, GRAY, shader);
	terrain_mat.shader = shader;
	// only one uv rect lookup option, which just shows the whole texture
	static const Rectangle basic_uv_rect[] = {{0, 0, 1, 1}};
	for (uint8_t i = 0; i < NUM_THREADS; ++i) {
		threaded_voxel_data[i].uv_rect_lookup = basic_uv_rect;
		threaded_voxel_data[i].uv_rect_lookup_capacity = 1;
	}

	// consider all player positions to be "dirty": ie chunks need to be loaded
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		player_positions[i].dirty = true;
	}

	terrain_update_chunks();
}

void terrain_update_player_pos(uint8_t index, Vector3 pos)
{
	assert(index < NUM_PLANES);
	player_positions[index].prev_coords = player_positions[index].coords;
	player_positions[index].coords = pos;

	// now maybe mark as dirty if the plane has moved enough
	{
		ChunkCoords old_chunk = {
			.x = (chunk_index_t)ceilf(player_positions[index].prev_coords.x /
									  (float)CHUNK_SIZE),
			.z = (chunk_index_t)ceilf(player_positions[index].prev_coords.z /
									  (float)CHUNK_SIZE)};
		ChunkCoords current_chunk = {
			.x = (chunk_index_t)ceilf(player_positions[index].coords.x /
									  (float)CHUNK_SIZE),
			.z = (chunk_index_t)ceilf(player_positions[index].coords.z /
									  (float)CHUNK_SIZE)};

		player_positions[index].dirty = (old_chunk.x != current_chunk.x) ||
										(old_chunk.z != current_chunk.z);
	}
}

void terrain_cleanup()
{
	for (size_t i = 0; i < terrain_data->count; ++i) {
		UnloadMesh(terrain_data->chunks[i].mesh);
	}
	UnloadMaterial(terrain_mat);
	// not necessary in theory, material should unload the RT. just bein safe
	UnloadRenderTexture(texture_atlas);
	RL_FREE(terrain_data->available_indices);
	RL_FREE(terrain_data);
	RL_FREE(player_positions);
	RL_FREE(threads);
	RL_FREE(threaded_voxel_data);
}

void terrain_update() { terrain_update_chunks(); }

/// NOTE: this is multithreaded in that we can load multiple chunks at the same
/// time. however there is not a separate terrain thread, meaning we have to
/// wait for all the loading jobs to finish before terrain_update completes.
static void terrain_update_chunks()
{
	typedef struct
	{
		uint8_t size;
		uint8_t indices[NUM_PLANES];
	} DirtyList;

	// create list of planes which need updates
	DirtyList dirty_planes;
	dirty_planes.size = 0;
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		if (player_positions[i].dirty) {
			dirty_planes.indices[dirty_planes.size] = i;
			++dirty_planes.size;
		}
	}

	if (dirty_planes.size == 0) {
		return;
	}

	// planes have moved, some chunks need to be reloaded. first, establish what
	// chunks we need

	// this list could potentially grow to maximum size (if we teleport both
	// planes far apart). however usually it is very small, RENDER_DISTANCE in
	// the most common case, twice that potentially if both planes move over a
	// chunk boundary on the same frame. could grow larger if
	// update_plane_position is called rarely, such that planes could cross
	// multiple chunk boundaries before we notice.
	static UnneededChunkList unneeded;
	unneeded.size = 0;

	for (uint8_t plane_index = 0; plane_index < dirty_planes.size;
		 ++plane_index) {
		uint8_t actual_index = dirty_planes.indices[plane_index];
		// for this plane, loop through all chunks and mark unneeded ones.
		for (size_t i = 0; i < terrain_data->count; ++i) {
			Chunk* chunk = &terrain_data->chunks[i];
			uint32_t xdiff =
				abs((int32_t)chunk->position.x -
					(int32_t)(player_positions[actual_index].coords.x /
							  CHUNK_SIZE));
			uint32_t zdiff =
				abs((int32_t)chunk->position.z -
					(int32_t)(player_positions[actual_index].coords.z /
							  CHUNK_SIZE));
			if (xdiff > RENDER_DISTANCE_HALF || zdiff > RENDER_DISTANCE_HALF) {
				assert(chunk->loaders > 0);
				chunk->loaders -= 1;
				if (chunk->loaders == 0) {
					unneeded.indices[unneeded.size] = i;
					++unneeded.size;
					assert(unneeded.size <= sizeof(unneeded.indices) /
												sizeof(unneeded.indices[0]));
				}
			}
		}

		player_positions[actual_index].dirty = false;
	}

	TraceLog(LOG_INFO, "removing %d chunks", unneeded.size);

	// mark these indices as available for overwrite within the terrain_data
	// pool
	terrain_data_mark_indices_free(terrain_data, &unneeded);

	// now start a bunch of threads, each generating chunks into this queue.
	// when out of threads, wait on the next one. when all thre
	Chunk insertion_queue[NUM_THREADS];
	size_t chunks_added = 0;
	ChunkCoords chunk_location = {-RENDER_DISTANCE_HALF, -RENDER_DISTANCE_HALF};
	uint8_t thread_index = 0;
	for (; chunk_location.x < RENDER_DISTANCE_HALF; ++chunk_location.x) {
		for (chunk_location.z = -RENDER_DISTANCE_HALF;
			 chunk_location.z < RENDER_DISTANCE_HALF; ++chunk_location.z) {

			// if the chunk is already loaded, just add to its loaders count
			bool loaded = false;
			for (size_t i = 0; i < terrain_data->count; ++i) {
				ChunkCoords iter = terrain_data->chunks[i].position;
				if (iter.x == chunk_location.x && iter.z == chunk_location.z) {
					loaded = true;
					if (terrain_data->chunks[i].loaders == 0) {
						TraceLog(LOG_WARNING,
								 "Re-loading chunk that seems to have been "
								 "unloaded this frame. Were there chunks left "
								 "over from last frame?");
					}
					terrain_data->chunks[i].loaders += 1;
					break;
				}
			}

			if (loaded) {
				continue;
			}

			if (threads[thread_index].working) {
				// this thread was already doing something, we just have to wait
				pthread_join(threads[thread_index].pthread, NULL);
				threads[thread_index].working = false;
				// which means it has something to submit
				size_t inserted_index = terrain_mesh_insert(
					terrain_data, &insertion_queue[thread_index]);
				chunks_added++;
			}

			Chunk* chunk_to_generate = &insertion_queue[thread_index];
			chunk_to_generate->loaders = 1;
			// offload actual chunk generation to other thread
			terrain_start_generation_job(chunk_location, thread_index,
										 chunk_to_generate);

			// move on to the next thread
			++thread_index;
			thread_index %= NUM_THREADS;
		}
	}

	// join all threads
	for (uint8_t i = 0; i < NUM_THREADS; ++i) {
		if (threads[i].working) {
			// this thread was already doing something, we just have to wait
			pthread_join(threads[i].pthread, NULL);
			threads[i].working = false;
			// which means it has something to submit
			size_t inserted_index =
				terrain_mesh_insert(terrain_data, &insertion_queue[i]);
			chunks_added++;
		}
	}

	TraceLog(LOG_INFO, "added %d chunks", chunks_added);

	terrain_data_normalize(terrain_data);
	assert(terrain_data->available_indices->count == 0);

	TraceLog(LOG_DEBUG, "Created %d chunk meshes", terrain_data->count);
}

static void terrain_start_generation_job(ChunkCoords chunk_coords,
										 uint8_t thread_index, Chunk* out_chunk)
{
	threads[thread_index].working = true;
	generation_jobs[thread_index].chunk_coords = chunk_coords;
	generation_jobs[thread_index].thread_index = thread_index;
	generation_jobs[thread_index].out_chunk = out_chunk;
	pthread_create(&threads[thread_index].pthread, NULL,
				   terrain_generate_mesh_for_chunk_thread_wrapper,
				   &generation_jobs[thread_index]);
}

static void* terrain_generate_mesh_for_chunk_thread_wrapper(void* args)
{
	GenerationArgs* realargs = args;
	terrain_generate_mesh_for_chunk(realargs);
	return NULL;
}

// TODO: implement pooling in mesher_allocate so that we dont have every thread
// allocating and then immediately deallocating large buffers of vertex data
static void terrain_generate_mesh_for_chunk(GenerationArgs* args)
{
	IntermediateVoxelData* voxel_buffer =
		&threaded_voxel_data[args->thread_index];
	voxel_buffer->coords = args->chunk_coords;
	terrain_voxel_data_generate(voxel_buffer);
	// voxels are now filled with the correct block_t values, meshing
	// time
	Mesher mesher;
	mesher_create(&mesher);
	// pass number of faces into "quads" argument of allocate, since all
	// the faces are quads (these are cubes)
	size_t faces = terrain_voxel_data_get_face_count(voxel_buffer);
	mesher_allocate(&mesher, faces);
	terrain_voxel_data_populate_mesher(voxel_buffer, &mesher);
	Mesh mesh = mesher_release(&mesher);
	UploadMesh(&mesh, false);

	// mesh is now on the GPU, go ahead and free the cpu parts
	RL_FREE(mesh.vertices);
	RL_FREE(mesh.texcoords);
	RL_FREE(mesh.normals);
	assert(mesh.texcoords2 == NULL);
	assert(mesh.colors == NULL);
	assert(mesh.indices == NULL);
	mesh.vertices = NULL;
	mesh.normals = NULL;
	mesh.texcoords = NULL;

	// mesh has been modified to contain handles from the opengl context
	// send it to the output
	args->out_chunk->position = args->chunk_coords;
	args->out_chunk->mesh = mesh;
}
