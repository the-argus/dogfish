#include "terrain_internal.h"
#include "quicksort.h"
#include "threadutils.h"
#include <FastNoiseLite.h>

static fnl_state terrain_noise_perlin_main;

static const VoxelCoords max_voxelcoord = {
	.x = CHUNK_SIZE,
	.y = WORLD_HEIGHT,
	.z = CHUNK_SIZE,
};

float perlin_3d(float x, float y, float z)
{
	float gen = fnlGetNoise3D(&terrain_noise_perlin_main, x, y, z);
	const float amplitude3d = powf(terrain_noise_perlin_main.gain,
								   (float)terrain_noise_perlin_main.octaves);
	// squash generated noise to be roughly between 0 and 1
	float squashed = gen / amplitude3d;
	return squashed;
}

float perlin_2d(float x, float y)
{
	return fnlGetNoise2D(&terrain_noise_perlin_main, x, y);
}

void init_noise()
{
	terrain_noise_perlin_main = fnlCreateState();
	terrain_noise_perlin_main.octaves = 8;
	terrain_noise_perlin_main.noise_type = FNL_NOISE_PERLIN;
	terrain_noise_perlin_main.lacunarity = 2;
}

VoxelCoords terrain_add_offset_to_voxel_coord(VoxelCoords coords,
											  const VoxelOffset offset)
{
	switch (offset.axis) {
	case AXIS_X:
		coords.x += offset.negative ? -1 : 1;
		break;
	case AXIS_Y:
		coords.y += offset.negative ? -1 : 1;
		break;
	case AXIS_Z:
		coords.z += offset.negative ? -1 : 1;
		break;
	default:
		TraceLog(LOG_WARNING, "invalid voxel offset axis");
#ifndef NDEBUG
		threadutils_exit(EXIT_FAILURE);
#endif
	}

	assert(coords.x >= 0 && coords.x < max_voxelcoord.x);
	assert(coords.y >= 0 && coords.y < max_voxelcoord.y);
	assert(coords.y >= 0 && coords.z < max_voxelcoord.z);

	return coords;
}

size_t terrain_mesh_insert(TerrainData* restrict data, const Chunk* new_chunk)
{
	if (!(data->count <= data->capacity)) {
		TraceLog(LOG_INFO, "data count %d and available indices %d",
				 data->count, data->available_indices->count);
	}
	assert(data->count <= data->capacity);
	size_t index = data->count;

	// only add to the end if there are no available spots
	if (data->available_indices->count == 0) {
		// preemptively add to the count of data
		++data->count;
	} else {
		index = data->available_indices
					->indices[data->available_indices->count - 1];
		--data->available_indices->count;
	}
	data->chunks[index] = *new_chunk;
	return index;
}

void terrain_data_mark_indices_free(TerrainData* restrict data,
									const UnneededChunkList* restrict unneeded)
{
	// append all of the unneededs to available indices
	for (size_t i = 0; i < unneeded->size; ++i) {
		data->available_indices->indices[data->available_indices->count] =
			unneeded->indices[i];
		++data->available_indices->count;
		assert(data->available_indices->count <=
			   data->available_indices->capacity);
	}
}

void terrain_data_normalize(TerrainData* data)
{
	if (data->available_indices->count == 0) {
		return;
	}

	// remove in order
	quicksort_inplace_size_t(&data->available_indices->indices[0],
							 data->available_indices->count, sizeof(size_t));

	// the largest available index should not be greater than the number of
	// chunks
	assert(
		data->available_indices->indices[data->available_indices->count - 1] <
		data->count);
	assert(data->available_indices->count <= data->count);

#ifndef NDEBUG
	for (size_t i = 0; i < data->available_indices->count; ++i) {
		assert(data->chunks[data->available_indices->indices[i]].loaders == 0);
	}
#endif

	for (int i = (int)data->available_indices->count - 1; i >= 0; --i) {
		// starting with the largest available index
		size_t available = data->available_indices->indices[i];
		if (available == data->count - 1) {
			// the last item in the array of chunks needs to be removed
			--data->count;
			continue;
		}
		// otherwise, we can move this one
		data->chunks[available] = data->chunks[data->count - 1];
		--data->count;
	}

	// all indices should be removed
	data->available_indices->count = 0;
}

block_t terrain_generate_voxel(ChunkCoords chunk, VoxelCoords voxel)
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

bool terrain_voxel_is_solid(block_t type) { return type != 0; }

void terrain_mesher_add_face(Mesher* restrict mesher,
							 const Vector3* restrict position,
							 const VoxelFaceInfo* restrict face)
{
	mesher->normal = face->normal;
	for (uint8_t i = 0; i < NUM_SIDES; ++i) {
		mesher->uv = face->vertex_infos[i].uv;
		mesher_push_vertex(mesher, &face->vertex_infos[i].offset, position);
	}
}

void terrain_add_voxel_to_mesher(Mesher* restrict mesher, VoxelCoords coords,
								 ChunkCoords chunk_coords, VoxelFaces faces,
								 const Rectangle* restrict uv_rect_lookup,
								 block_t voxel)
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
