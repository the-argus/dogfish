#include "terrain_internal.h"
#include "constants/general.h"
#include "mesher.h"
#include "rlights.h"
#include "terrain.h"
#include "terrain_render.h"
#include "terrain_voxel_data.h"
#include "threadutils.h"
#include <math.h>
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

static TerrainData* terrain_data;
static Material terrain_mat;
static PlayerPosition* player_positions;
static RenderTexture texture_atlas;
static IntermediateVoxelData* voxel_data;

static void terrain_generate_mesh_for_chunk(ChunkCoords chunk_coords,
											Chunk* out_chunk);

static void terrain_update_chunks();

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
	size_t num_meshes =
		(size_t)(RENDER_DISTANCE + 1) * RENDER_DISTANCE * NUM_PLANES;
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
	voxel_data = RL_CALLOC(1, sizeof(IntermediateVoxelData));

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
	voxel_data->uv_rect_lookup = basic_uv_rect;
	voxel_data->uv_rect_lookup_capacity = 1;

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
	RL_FREE(voxel_data);
}

void terrain_update() { terrain_update_chunks(); }

/// NOTE: this is multithreaded in that we can load multiple chunks at the same
/// time. however there is not a separate terrain thread, meaning we have to
/// wait for all the loading jobs to finish before terrain_update completes.
static void terrain_update_chunks()
{
	// check if we have any dirty positions before doing unloading/loading
	{
		size_t total_dirty = 0;
		for (size_t i = 0; i < NUM_PLANES; ++i) {
			total_dirty += player_positions[i].dirty;
		}
		if (total_dirty == 0) {
			return;
		}
	}

	// mark all chunks as having 0 loaders. so by default they will be unloaded
	for (size_t i = 0; i < terrain_data->count; ++i) {
		terrain_data->chunks[i].loaders = 0;
	}

	// for each plane, add 1 to nearby chunks or load them if they don't exist.
	for (uint8_t plane_index = 0; plane_index < NUM_PLANES; ++plane_index) {
		size_t chunks_added = 0;
		ChunkCoords player_location = {
			.x = (chunk_index_t)(player_positions[plane_index].coords.x /
								 CHUNK_SIZE),
			.z = (chunk_index_t)(player_positions[plane_index].coords.z /
								 CHUNK_SIZE),
		};
		ChunkCoords chunk_location;

		// annoyingly complex looking for loop to go through all the chunk
		// coordinates that the player wants to load
		for (chunk_location.x =
				 (chunk_index_t)(player_location.x - RENDER_DISTANCE_HALF);
			 chunk_location.x <
			 (chunk_index_t)(player_location.x + RENDER_DISTANCE_HALF);
			 ++chunk_location.x) {
			for (chunk_location.z =
					 (chunk_index_t)(player_location.z - RENDER_DISTANCE_HALF);
				 chunk_location.z <
				 (chunk_index_t)(player_location.z + RENDER_DISTANCE_HALF);
				 ++chunk_location.z) {

				// if the chunk is already loaded, just add to its loaders count
				bool loaded = false;
				for (size_t i = 0; i < terrain_data->count; ++i) {
					ChunkCoords iter = terrain_data->chunks[i].position;
					if (iter.x == chunk_location.x &&
						iter.z == chunk_location.z) {
						loaded = true;
						terrain_data->chunks[i].loaders += 1;
						break;
					}
				}

				if (loaded) {
					continue;
				}

				Chunk out;
				terrain_generate_mesh_for_chunk(chunk_location, &out);
				terrain_mesh_insert(terrain_data, &out);
				++chunks_added;
			}
		}
		TraceLog(LOG_INFO, "added %d chunks", chunks_added);

		player_positions[plane_index].dirty = false;
	}

	// this list could potentially grow to maximum size (if we teleport both
	// planes far apart). however usually it is very small, RENDER_DISTANCE in
	// the most common case, twice that potentially if both planes move over a
	// chunk boundary on the same frame. could grow larger if
	// update_plane_position is called rarely, such that planes could cross
	// multiple chunk boundaries before we notice.
	static UnneededChunkList unneeded;
	unneeded.size = 0;
	for (size_t i = 0; i < terrain_data->count; ++i) {
		if (terrain_data->chunks[i].loaders == 0) {
			unneeded.indices[unneeded.size] = i;
			++unneeded.size;
		}
	}

	TraceLog(LOG_INFO, "removing %d chunks", unneeded.size);

	terrain_data_mark_indices_free(terrain_data, &unneeded);

	terrain_data_normalize(terrain_data);
	assert(terrain_data->available_indices->count == 0);

	TraceLog(LOG_DEBUG, "Created %d chunk meshes", terrain_data->count);
}

static void terrain_generate_mesh_for_chunk(ChunkCoords chunk_coords,
											Chunk* out_chunk)
{
	voxel_data->coords = chunk_coords;
	terrain_voxel_data_generate(voxel_data);
	// voxels are now filled with the correct block_t values, meshing
	// time
	Mesher mesher;
	mesher_create(&mesher);
	// pass number of faces into "quads" argument of allocate, since all
	// the faces are quads (these are cubes)
	size_t faces = terrain_voxel_data_get_face_count(voxel_data);
	mesher_allocate(&mesher, faces);
	terrain_voxel_data_populate_mesher(voxel_data, &mesher);
	Mesh mesh = mesher_release(&mesher);

	{
		Mesh* available_mesh = NULL;

		// if there is a mesh available, just use its VAO and VBO etc
		if (terrain_data->available_indices->count > 0) {
			available_mesh =
				&terrain_data
					 ->chunks[terrain_data->available_indices->indices
								  [terrain_data->available_indices->count - 1]]
					 .mesh;
		}

		UploadTerrainMesh(&mesh, available_mesh, false);

		// set the mesh's VAO and VBOs to 0 so they dont get cleared on unload
		// (we are now using those handles)
		if (available_mesh) {
			terrain_render_clear_mesh(available_mesh);
		}
	}

	// mesh is now on the GPU, go ahead and free the cpu parts
	RL_FREE(mesh.vertices);
	RL_FREE(mesh.texcoords);
	RL_FREE(mesh.normals);
	assert(mesh.texcoords2 == NULL);
	assert(mesh.colors == NULL);
	mesh.vertices = NULL;
	mesh.normals = NULL;
	mesh.texcoords = NULL;
	mesh.indices = NULL;

	// mesh has been modified to contain handles from the opengl context
	// send it to the output
	out_chunk->position = chunk_coords;
	out_chunk->mesh = mesh;
}
