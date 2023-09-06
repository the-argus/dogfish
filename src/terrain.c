#include "terrain_internal.h"
#include "constants/general.h"
#include "mesher.h"
#include "rlights.h"
#include "terrain.h"
#include "threadutils.h"
#include <math.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

// TODO: make the wording in this module more consistent/clear: what is a chunk
// vs. voxels

static TerrainData* terrain_data;
static Material terrain_mat;
static Vector3* player_positions;
static RenderTexture texture_atlas;

static const VoxelCoords max_voxelcoord = {
	.x = CHUNK_SIZE,
	.y = WORLD_HEIGHT,
	.z = CHUNK_SIZE,
};

enum Sides : uint8_t
{
	SOUTH = 0,
	NORTH,
	WEST,
	EAST,
	UP,
	DOWN,
	NUM_SIDES
};

static const VoxelOffset all_offsets[NUM_SIDES] = {
	(VoxelOffset){.axis = AXIS_Z, .negative = false},
	(VoxelOffset){.axis = AXIS_Z, .negative = true},
	(VoxelOffset){.axis = AXIS_X, .negative = false},
	(VoxelOffset){.axis = AXIS_X, .negative = true},
	(VoxelOffset){.axis = AXIS_Y, .negative = false},
	(VoxelOffset){.axis = AXIS_Y, .negative = true},
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
static void terrain_generate_voxels(IntermediateVoxelData* voxels);
// get the block_t for a single voxel given its coordinates
static block_t terrain_generate_voxel(ChunkCoords chunk, VoxelCoords voxel);
static size_t terrain_count_faces_in_chunk(const IntermediateVoxelData* voxels);
static void terrain_add_voxel_to_mesher(
	Mesher* restrict mesher, VoxelCoords coords, ChunkCoords chunk_coords,
	VoxelFaces faces, const Rectangle* restrict uv_rect_lookup, block_t voxel);
/// Check if a voxel at `offset` voxels away from `coords` is solid or not. If
/// Overflow past the end of the chunk occurs, then the block is considered not
/// solid.
static bool
terrain_offset_voxel_is_solid(const IntermediateVoxelData* chunk_data,
							  VoxelCoords coords, VoxelOffset offset);

static void terrain_mesher_add_face(Mesher* restrict mesher,
									const Vector3* restrict position,
									const VoxelFaceInfo* restrict face);

/// Inserts a new mesh at a given coordinates into a TerrainMeshes
/// returns the index at which the item was inserted
static void terrain_mesh_insert(TerrainData* restrict data,
								ChunkCoords chunk_location,
								const Mesh* restrict mesh);

static void
terrain_chunk_to_mesh(Mesher* restrict mesher, ChunkCoords chunk_coords,
					  const IntermediateVoxelData* restrict chunk_data);

static bool terrain_voxel_is_solid(block_t type);

void terrain_draw()
{
	for (size_t i = 0; i < terrain_data->count; ++i) {
		const ChunkCoords* pos = &terrain_data->chunks[i].position;
		DrawMesh(terrain_data->chunks[i].mesh, terrain_mat,
				 MatrixTranslate((float)pos->x * (0.8f / CHUNK_SIZE), 0,
								 (float)pos->z * (0.8f / CHUNK_SIZE)));
	}
}

void terrain_load()
{
	init_noise();
	// permanent
	size_t num_meshes = (size_t)RENDER_DISTANCE * RENDER_DISTANCE * NUM_PLANES;
	terrain_data = RL_MALLOC(sizeof(TerrainData) +
							 (num_meshes * sizeof(terrain_data->chunks[0])));
	terrain_data->capacity = num_meshes;
	terrain_data->count = 0;
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
	voxels->uv_rect_lookup = basic_uv_rect;
	voxels->uv_rect_lookup_capacity = 1;

	// start in the most negative chunk from the player
	ChunkCoords chunk_location = {-RENDER_DISTANCE_HALF, -RENDER_DISTANCE_HALF};
	for (; chunk_location.x < RENDER_DISTANCE_HALF; ++chunk_location.x) {
		for (chunk_location.z = -RENDER_DISTANCE_HALF;
			 chunk_location.z < RENDER_DISTANCE_HALF; ++chunk_location.z) {
			voxels->coords = chunk_location;
			terrain_generate_voxels(voxels);
			// voxels are now filled with the correct block_t values, meshing
			// time
			// TODO: make this stack allocated, at least in release mode
			Mesher* mesher = RL_MALLOC(sizeof(Mesher));
			mesher_create(mesher);
			// pass number of faces into "quads" argument of allocate, since all
			// the faces are quads (these are cubes)
			size_t faces = terrain_count_faces_in_chunk(voxels);
			mesher_allocate(mesher, faces);
			terrain_chunk_to_mesh(mesher, chunk_location, voxels);
			Mesh mesh = mesher_release(mesher);
			UploadMesh(&mesh, false);
			// mesh has been modified to contain handles from the opengl context
			// now copy it into our data structure for rendering
			terrain_mesh_insert(terrain_data, chunk_location, &mesh);
			RL_FREE(mesher);
		}
	}
	TraceLog(LOG_DEBUG, "Created %d chunk meshes", terrain_data->count);
	RL_FREE(voxels);
}

void terrain_update_player_pos(uint8_t index, Vector3 pos)
{
	assert(index < NUM_PLANES);
}

void terrain_cleanup()
{
	for (size_t i = 0; i < terrain_data->count; ++i) {
		UnloadMesh(terrain_data->chunks[i].mesh);
	}
	UnloadMaterial(terrain_mat);
	// not necessary in theory, material should unload the RT. just bein safe
	UnloadRenderTexture(texture_atlas);
	RL_FREE(terrain_data);
	RL_FREE(player_positions);
}

static void terrain_mesh_insert(TerrainData* restrict data,
								ChunkCoords chunk_location,
								const Mesh* restrict mesh)
{
	assert(data->count < data->capacity);
	uint16_t index = data->count;

	data->chunks[index].mesh = *mesh;
	data->chunks[index].position = chunk_location;
	++data->count;
}

static void
terrain_chunk_to_mesh(Mesher* restrict mesher, ChunkCoords chunk_coords,
					  const IntermediateVoxelData* restrict chunk_data)
{
	VoxelCoords iter = {0};
	for (iter.x = 0; iter.x < max_voxelcoord.x; ++iter.x) {
		for (iter.y = 0; iter.y < max_voxelcoord.y; ++iter.y) {
			for (iter.z = 0; iter.z < max_voxelcoord.z; ++iter.z) {
				if (*terrain_get_voxel_from_voxels_const(iter, chunk_data) ==
					0) {
					// empty
					continue;
				}
				VoxelFaces faces = {
					.south = !terrain_offset_voxel_is_solid(chunk_data, iter,
															all_offsets[SOUTH]),
					.north = !terrain_offset_voxel_is_solid(chunk_data, iter,
															all_offsets[NORTH]),
					.west = !terrain_offset_voxel_is_solid(chunk_data, iter,
														   all_offsets[WEST]),
					.east = !terrain_offset_voxel_is_solid(chunk_data, iter,
														   all_offsets[EAST]),
					.up = !terrain_offset_voxel_is_solid(chunk_data, iter,
														 all_offsets[UP]),
					.down = !terrain_offset_voxel_is_solid(chunk_data, iter,
														   all_offsets[DOWN]),
				};

				block_t voxel =
					*terrain_get_voxel_from_voxels_const(iter, chunk_data);

				terrain_add_voxel_to_mesher(mesher, iter, chunk_coords, faces,
											chunk_data->uv_rect_lookup, voxel);
			}
		}
	}
}

static void terrain_generate_voxels(IntermediateVoxelData* voxels)
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
		for (iter.y = 0; iter.y < max_voxelcoord.y; ++iter.y) {
			for (iter.z = 0; iter.z < max_voxelcoord.z; ++iter.z) {
				*terrain_get_voxel_from_voxels(iter, voxels) =
					terrain_generate_voxel(voxels->coords, iter);
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
	const size_t val =
		voxel.z + voxel.x * max_voxelcoord.x + voxel.y * max_voxelcoord.y;
	static const size_t max = (long)CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT;
	assert(val < max);
	return &voxels->voxels[val];
}

static block_t terrain_generate_voxel(ChunkCoords chunk, VoxelCoords voxel)
{
	const float perlin = perlin_3d(
		(float)voxel.x + (float)(chunk.x * max_voxelcoord.x), (float)voxel.y,
		(float)voxel.z + (float)(chunk.z * max_voxelcoord.z));
	// TraceLog(LOG_INFO,
	// 		 "Expected perlin with amplitude of %f, got %f. Squashed to %f",
	// 		 amplitude3d, perlin, squashed_perlin);
	// assert(squashed_perlin <= 1 && squashed_perlin >= -1);

	if (perlin < 0) {
		return 1;
	}
	return 0;
}

static size_t terrain_count_faces_in_chunk(const IntermediateVoxelData* voxels)
{
	size_t count = 0;
	VoxelCoords iter = {0};
	for (; iter.x < max_voxelcoord.x; ++iter.x) {
		for (iter.y = 0; iter.y < max_voxelcoord.y; ++iter.y) {
			for (iter.z = 0; iter.z < max_voxelcoord.z; ++iter.z) {
				const block_t* voxel =
					terrain_get_voxel_from_voxels_const(iter, voxels);

				if (!terrain_voxel_is_solid(*voxel)) {
					// empty
					continue;
				}

				// otherwise we need to count the number of solid neighbors
				for (uint8_t i = 0; i < NUM_SIDES; ++i) {
					// if the neighbor isn't solid, that means the  face is
					// exposed and needs to be rendered
					if (!terrain_offset_voxel_is_solid(voxels, iter,
													   all_offsets[i])) {
						++count;
					}
				}
			}
		}
	}

	return count;
}

static bool
terrain_offset_voxel_is_solid(const IntermediateVoxelData* chunk_data,
							  VoxelCoords coords, VoxelOffset offset)
{
	VoxelCoords offset_coords = coords;
	ChunkCoords overflow_chunk = chunk_data->coords;

	switch (offset.axis) {
	case AXIS_X:
		if (offset.negative && coords.x == 0) {
			// overflow to negative x
			--overflow_chunk.x;
			offset_coords.x = max_voxelcoord.x - 1;
		} else if (!offset.negative && coords.x == max_voxelcoord.x - 1) {
			// overflow to positive x
			++overflow_chunk.x;
			offset_coords.x = 0;
		} else {
			// usual case, just move to an adjacent voxel
			offset_coords.x += offset.negative ? -1 : 1;
		}
		break;
	case AXIS_Y:
		// ALL off-chunk y values are considered solid, to avoid generating
		// faces at the top and bottom of the world
		if ((offset.negative && coords.y == 0) ||
			(!offset.negative && coords.y == max_voxelcoord.y - 1)) {
			return true;
		} else {
			offset_coords.y += offset.negative ? -1 : 1;
		}
		break;
	case AXIS_Z:
		if (offset.negative && coords.z == 0) {
			--overflow_chunk.z;
			offset_coords.z = max_voxelcoord.z - 1;
		} else if (!offset.negative && coords.z == max_voxelcoord.z - 1) {
			++overflow_chunk.z;
			offset_coords.z = 0;
		} else {
			offset_coords.z += offset.negative ? -1 : 1;
		}
		break;
	default:
		TraceLog(LOG_WARNING, "invalid voxel offset axis");
#ifndef NDEBUG
		threadutils_exit(EXIT_FAILURE);
#endif
		break;
	}

	if (overflow_chunk.x != chunk_data->coords.x ||
		overflow_chunk.z != chunk_data->coords.z) {
		// assert that we actually didnt overflow
		assert(offset_coords.x >= 0 && offset_coords.x < max_voxelcoord.x);
		assert(offset_coords.y >= 0 && offset_coords.y < max_voxelcoord.y);
		assert(offset_coords.z >= 0 && offset_coords.z < max_voxelcoord.z);

		// if we did overflow, we need to generate the value of the other
		// chunk's blocks because we don't know it (this function only recieves
		// the current chunk's data)
		const block_t voxel =
			terrain_generate_voxel(overflow_chunk, offset_coords);
		return terrain_voxel_is_solid(voxel);
	}

	const block_t voxel =
		*terrain_get_voxel_from_voxels_const(offset_coords, chunk_data);
	return terrain_voxel_is_solid(voxel);
}

static bool terrain_voxel_is_solid(block_t type) { return type != 0; }

static void terrain_mesher_add_face(Mesher* restrict mesher,
									const Vector3* restrict position,
									const VoxelFaceInfo* restrict face)
{
	mesher->normal = face->normal;
	for (uint8_t i = 0; i < NUM_SIDES; ++i) {
		mesher->uv = face->vertex_infos[i].uv;
		mesher_push_vertex(mesher, &face->vertex_infos[i].offset, position);
	}
}

static void terrain_add_voxel_to_mesher(
	Mesher* restrict mesher, VoxelCoords coords, ChunkCoords chunk_coords,
	VoxelFaces faces, const Rectangle* restrict uv_rect_lookup, block_t voxel)
{
	const Rectangle* uv_rect = &uv_rect_lookup[voxel];
	const Vector3 rl_coords = (Vector3){
		(float)((voxel_index_signed_t)coords.x +
				(chunk_coords.x * max_voxelcoord.x)),
		(float)coords.y,
		(float)((voxel_index_signed_t)coords.z +
				(chunk_coords.z * max_voxelcoord.z)),
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
