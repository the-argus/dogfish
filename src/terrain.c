#include "terrain_internal.h"
#include "constants/general.h"
#include "mesher.h"
#include "rlights.h"
#include "terrain.h"
#include <math.h>
#include <raymath.h>
#include <stdlib.h>

// TODO: make the wording in this module more consistent/clear: what is a chunk
// vs. voxels

static TerrainMeshes* terrain_meshes;
static Material terrain_mat;
static Vector3* player_positions;
static RenderTexture texture_atlas;

static const VoxelCoords max_voxelcoord = {
	.x = CHUNK_SIZE,
	.y = WORLD_HEIGHT,
	.z = CHUNK_SIZE,
};

#define SOUTH 0
#define NORTH 1
#define WEST 2
#define EAST 3
#define UP 4
#define DOWN 5
static const VoxelOffset all_neighbor_offsets[] = {
	(VoxelOffset){.x = 0, .y = 0, .z = 1},
	(VoxelOffset){.x = 0, .y = 0, .z = -1},
	(VoxelOffset){.x = 1, .y = 0, .z = 0},
	(VoxelOffset){.x = -1, .y = 0, .z = 0},
	(VoxelOffset){.x = 0, .y = 1, .z = 0},
	(VoxelOffset){.x = 0, .y = -1, .z = 0},
};

/// Typedef used to record a single bit per face of a voxel. Useful for storing
/// information about what sides of a voxel are visible.
typedef struct
{
	uint8_t south : 1;
	uint8_t north : 1;
	uint8_t west : 1;
	uint8_t east : 1;
	uint8_t up : 1;
	uint8_t down : 1;
} VoxelFaces;

// helpers
static void terrain_chunk_mesh_allocate(Mesh* mesh, size_t triangle_count);
/// Get a voxel from a bunch of voxel data
static block_t* terrain_get_voxel_from_voxels(VoxelCoords voxel,
											  IntermediateVoxelData* voxels);
static const block_t*
terrain_get_voxel_from_voxels_const(VoxelCoords voxel,
									const IntermediateVoxelData* voxels);
// fill voxels with block_ts based on perlin noise values
static void terrain_generate_voxels(ChunkCoords chunk_to_generate,
									IntermediateVoxelData* voxels);
// get the block_t for a single voxel given its coordinates
static block_t terrain_generate_voxel(const ChunkCoords* restrict chunk,
									  const VoxelCoords* restrict voxel);
static size_t terrain_count_faces_in_chunk(const IntermediateVoxelData* voxels);
static void
terrain_add_voxel_to_mesher(Mesher* mesher, const VoxelCoords* coords,
							const ChunkCoords* chunk_coords, VoxelFaces faces,
							const Rectangle* uv_rect_lookup, block_t voxel);
/// Check if a voxel at `offset` voxels away from `coords` is solid or not. If
/// Overflow past the end of the chunk occurs, then the block is considered not
/// solid.
static bool
terrain_offset_voxel_is_solid(const IntermediateVoxelData* restrict chunk_data,
							  const VoxelCoords* restrict coords,
							  const VoxelOffset* restrict offset);

static void terrain_mesher_add_face(Mesher* restrict mesher,
									const Vector3* restrict position,
									const VoxelFaceInfo* restrict face);

static void
terrain_chunk_to_mesh(Mesher* restrict mesher,
					  const ChunkCoords* restrict chunk_coords,
					  const IntermediateVoxelData* restrict chunk_data);

void terrain_draw()
{
	ChunkCoords chunk = {-RENDER_DISTANCE / 2, -RENDER_DISTANCE / 2};
	size_t index = 0;
	for (; chunk.x < RENDER_DISTANCE; ++chunk.x) {
		for (; chunk.z < RENDER_DISTANCE; ++chunk.z) {
			DrawMesh(terrain_meshes->chunks[index], terrain_mat,
					 MatrixTranslate((float)(chunk.x * max_voxelcoord.x), 0,
									 (float)(chunk.z * max_voxelcoord.z)));
			++index;
		}
	}
}

void terrain_load()
{
	// permanent
	size_t num_meshes = (size_t)RENDER_DISTANCE * RENDER_DISTANCE * NUM_PLANES;
	terrain_meshes =
		RL_MALLOC(sizeof(TerrainMeshes) + (num_meshes * sizeof(Mesh)));
	terrain_meshes->size = num_meshes;
	player_positions = RL_CALLOC(NUM_PLANES, sizeof(Vector3));

	// heap allocated for debugging, could be stack'd
	IntermediateVoxelData* voxels = RL_CALLOC(1, sizeof(IntermediateVoxelData));

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
	Material mat = LoadMaterialDefault();
	mat.maps[0].color = WHITE;
	mat.maps[0].texture = texture_atlas.texture;

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
	mat.shader = shader;
	// only one uv rect lookup option, which just shows the whole texture
	static const Rectangle basic_uv_rect[] = {{0, 0, 1, 1}};
	voxels->uv_rect_lookup = basic_uv_rect;
	voxels->uv_rect_lookup_capacity = 1;

	// start in the most negative chunk from the player
	ChunkCoords chunk = {-RENDER_DISTANCE / 2, -RENDER_DISTANCE / 2};
	size_t index = 0;
	for (; chunk.x < RENDER_DISTANCE; ++chunk.x) {
		for (; chunk.z < RENDER_DISTANCE; ++chunk.z) {
			terrain_generate_voxels(chunk, voxels);
			// voxels are now filled with the correct block_t values, meshing
			// time
			Mesher mesher;
			mesher_create(&mesher);
			// pass number of faces into "quads" argument of allocate, since all
			// the faces are quads (these are cubes)
			mesher_allocate(&mesher, terrain_count_faces_in_chunk(voxels));

			assert(index < terrain_meshes->size);

			terrain_chunk_to_mesh(&mesher, &chunk, voxels);
			terrain_meshes->chunks[index] = mesher.inner;
			UploadMesh(&terrain_meshes->chunks[index], false);
			++index;
		}
	}
	RL_FREE(voxels);
}

void terrain_update_player_pos(uint8_t index, Vector3 pos)
{
	assert(index < NUM_PLANES);
}

void terrain_cleanup()
{
	for (size_t i = 0; i < terrain_meshes->size; ++i) {
		UnloadMesh(terrain_meshes->chunks[i]);
	}
	UnloadMaterial(terrain_mat);
	// not necessary in theory, material should unload the RT. just bein safe
	UnloadRenderTexture(texture_atlas);
	RL_FREE(terrain_meshes);
	RL_FREE(player_positions);
}

static void
terrain_chunk_to_mesh(Mesher* restrict mesher,
					  const ChunkCoords* restrict chunk_coords,
					  const IntermediateVoxelData* restrict chunk_data)
{
	VoxelCoords iter = {0};
	for (; iter.x < max_voxelcoord.x; ++iter.x) {
		for (; iter.y < max_voxelcoord.y; ++iter.y) {
			for (; iter.z < max_voxelcoord.z; ++iter.z) {
				VoxelFaces faces = {
					.south = terrain_offset_voxel_is_solid(
						chunk_data, &iter, &all_neighbor_offsets[SOUTH]),
					.north = terrain_offset_voxel_is_solid(
						chunk_data, &iter, &all_neighbor_offsets[NORTH]),
					.west = terrain_offset_voxel_is_solid(
						chunk_data, &iter, &all_neighbor_offsets[WEST]),
					.east = terrain_offset_voxel_is_solid(
						chunk_data, &iter, &all_neighbor_offsets[EAST]),
					.up = terrain_offset_voxel_is_solid(
						chunk_data, &iter, &all_neighbor_offsets[UP]),
					.down = terrain_offset_voxel_is_solid(
						chunk_data, &iter, &all_neighbor_offsets[DOWN]),
				};

				block_t voxel =
					*terrain_get_voxel_from_voxels_const(iter, chunk_data);

				terrain_add_voxel_to_mesher(mesher, &iter, chunk_coords, faces,
											chunk_data->uv_rect_lookup, voxel);
			}
		}
	}
}

static void terrain_generate_voxels(ChunkCoords chunk_to_generate,
									IntermediateVoxelData* voxels)
{
	VoxelCoords iter = {0};
	// TODO: this loop over voxels appears both here and in
	// terrain_count_faces_in_chunk. This is code duplication and bad cache
	// friendliness. Such a problem is also found in physics, where the two
	// were merged with physics_collide_and_move. we need a generic solution
	// to looping and attaching some handlers so that all the code can run
	// while the memory is hot. Provided that doing this actually makes things
	// faster, that is.
	for (; iter.x < max_voxelcoord.x; ++iter.x) {
		for (; iter.y < max_voxelcoord.y; ++iter.y) {
			for (; iter.z < max_voxelcoord.z; ++iter.z) {
				*terrain_get_voxel_from_voxels(iter, voxels) =
					terrain_generate_voxel(&chunk_to_generate, &iter);
			}
		}
	}
}

static block_t* terrain_get_voxel_from_voxels(VoxelCoords voxel,
											  IntermediateVoxelData* voxels)
{
	assert(voxel.z + voxel.x * max_voxelcoord.x + voxel.y * max_voxelcoord.y <
		   CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT);
	return &voxels->voxels[voxel.z + voxel.x * max_voxelcoord.x +
						   voxel.y * max_voxelcoord.y];
}

static const block_t*
terrain_get_voxel_from_voxels_const(VoxelCoords voxel,
									const IntermediateVoxelData* voxels)
{
	assert(voxel.z + voxel.x * max_voxelcoord.x + voxel.y * max_voxelcoord.y <
		   CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT);
	return &voxels->voxels[voxel.z + voxel.x * max_voxelcoord.x +
						   voxel.y * max_voxelcoord.y];
}

static block_t terrain_generate_voxel(const ChunkCoords* restrict chunk,
									  const VoxelCoords* restrict voxel)
{
	static const NoiseOptions noise3d = {
		.octaves = 8,
		.gain = 42,
		.hgrid = 2,
	};
	static const float threshhold = 0.0f;
	const float amplitude3d = powf(noise3d.gain, (float)noise3d.octaves);

	const float perlin = perlin_3d(
		(float)voxel->x + (float)(chunk->x * max_voxelcoord.x), (float)voxel->y,
		(float)voxel->z + (float)(chunk->z * max_voxelcoord.z), &noise3d);

	const float squashed_perlin = perlin / amplitude3d;
	assert(squashed_perlin <= 1 && squashed_perlin >= -1);

	if (squashed_perlin < 0) {
		return 1;
	}
	return 0;
}

static size_t terrain_count_faces_in_chunk(const IntermediateVoxelData* voxels)
{
	size_t count = 0;
	VoxelCoords iter = {0};
	static const size_t number_of_offsets =
		sizeof(all_neighbor_offsets) / sizeof(VoxelOffset);
	for (; iter.x < max_voxelcoord.x; ++iter.x) {
		for (; iter.y < max_voxelcoord.y; ++iter.y) {
			for (; iter.z < max_voxelcoord.z; ++iter.z) {
				const block_t* voxel =
					terrain_get_voxel_from_voxels_const(iter, voxels);

				if (*voxel == 0) {
					// empty
					continue;
				}

				// otherwise we need to count the number of solid neighbors
				for (uint8_t i = 0; i < number_of_offsets; ++i) {
					// if the neighbor isn't solid, that means the  face is
					// exposed and needs to be rendered
					if (terrain_offset_voxel_is_solid(
							voxels, &iter, &all_neighbor_offsets[i])) {
						++count;
					}
				}
			}
		}
	}

	return count;
}

static bool
terrain_offset_voxel_is_solid(const IntermediateVoxelData* restrict chunk_data,
							  const VoxelCoords* restrict coords,
							  const VoxelOffset* restrict offset)
{
	const voxel_index_signed_t x =
		((voxel_index_signed_t)coords->x) - offset->x;
	const voxel_index_signed_t y =
		((voxel_index_signed_t)coords->y) - offset->y;
	const voxel_index_signed_t z =
		((voxel_index_signed_t)coords->z) - offset->z;

	if (x < 0 || y < 0 || z < 0) {
		// all off-chunk values are not solid
		return false;
	}

	const block_t voxel = *terrain_get_voxel_from_voxels_const(
		(VoxelCoords){x, y, z}, chunk_data);
	return voxel == 0;
}

static void terrain_mesher_add_face(Mesher* restrict mesher,
									const Vector3* restrict position,
									const VoxelFaceInfo* restrict face)
{
	mesher->normal = face->normal;
	for (uint8_t i = 0; i < 6; ++i) {
		mesher->uv = face->vertex_infos[i].uv;
		mesher_push_vertex(mesher, &face->vertex_infos[i].offset, position);
	}
}

static void
terrain_add_voxel_to_mesher(Mesher* mesher, const VoxelCoords* coords,
							const ChunkCoords* chunk_coords, VoxelFaces faces,
							const Rectangle* uv_rect_lookup, block_t voxel)
{
	const Rectangle* uv_rect = &uv_rect_lookup[voxel];
	const Vector3 rl_coords = (Vector3){
		(float)((voxel_index_signed_t)coords->x +
				(chunk_coords->x * max_voxelcoord.x)),
		(float)coords->y,
		(float)((voxel_index_signed_t)coords->z +
				(chunk_coords->z * max_voxelcoord.z)),
	};

	// z-
	if (faces.north) {
		const VoxelFaceInfo north_face = {
			.normal = {0, 0, -1},
			.vertex_infos = {
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {1, 0, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &north_face);
	}

	// z+
	if (faces.south) {
		const VoxelFaceInfo south_face = {
			.normal = {0, 0, 1},
			.vertex_infos = {
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 1, 1}, .uv = {uv_rect->x, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &south_face);
	}

	// x+
	if (faces.west) {
		const VoxelFaceInfo west_face = {
			.normal = {1, 0, 0},
			.vertex_infos = {
				{.offset = {1, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &west_face);
	}

	// x-
	if (faces.east) {
		const VoxelFaceInfo east_face = {
			.normal = {-1, 0, 0},
			.vertex_infos = {
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {0, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &east_face);
	}

	if (faces.up) {
		const VoxelFaceInfo east_face = {
			.normal = {0, 1, 0},
			.vertex_infos = {
				{.offset = {0, 1, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {0, 1, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &east_face);
	}

	if (faces.down) {
		const VoxelFaceInfo east_face = {
			.normal = {0, -1, 0},
			.vertex_infos = {
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 0, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &east_face);
	}
}
