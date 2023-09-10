#include "terrain_voxel_data.h"
#include "threadutils.h"

static const VoxelCoords max_voxelcoord = {
	.x = CHUNK_SIZE,
	.y = WORLD_HEIGHT,
	.z = CHUNK_SIZE,
};

static const VoxelOffset all_offsets[NUM_SIDES] = {
	(VoxelOffset){.axis = AXIS_Z, .negative = false},
	(VoxelOffset){.axis = AXIS_Z, .negative = true},
	(VoxelOffset){.axis = AXIS_X, .negative = false},
	(VoxelOffset){.axis = AXIS_X, .negative = true},
	(VoxelOffset){.axis = AXIS_Y, .negative = false},
	(VoxelOffset){.axis = AXIS_Y, .negative = true},
};

/// Check if a voxel at `offset` voxels away from `coords` is solid or not. If
/// Overflow past the end of the chunk occurs, then the block is considered not
/// solid.
static bool
terrain_voxel_data_get_offset_is_solid(const IntermediateVoxelData* chunk_data,
									   VoxelCoords coords, VoxelOffset offset);

void terrain_voxel_data_populate_mesher(
	const IntermediateVoxelData* restrict chunk_data, Mesher* restrict mesher)
{
	VoxelCoords iter = {0};
	for (iter.x = 0; iter.x < max_voxelcoord.x; ++iter.x) {
		for (iter.y = 0; iter.y < max_voxelcoord.y; ++iter.y) {
			for (iter.z = 0; iter.z < max_voxelcoord.z; ++iter.z) {
				if (*terrain_voxel_data_get_const(chunk_data, iter) == 0) {
					// empty
					continue;
				}
				VoxelFaces faces = {
					.south = !terrain_voxel_data_get_offset_is_solid(
						chunk_data, iter, all_offsets[SOUTH]),
					.north = !terrain_voxel_data_get_offset_is_solid(
						chunk_data, iter, all_offsets[NORTH]),
					.west = !terrain_voxel_data_get_offset_is_solid(
						chunk_data, iter, all_offsets[WEST]),
					.east = !terrain_voxel_data_get_offset_is_solid(
						chunk_data, iter, all_offsets[EAST]),
					.up = !terrain_voxel_data_get_offset_is_solid(
						chunk_data, iter, all_offsets[UP]),
					.down = !terrain_voxel_data_get_offset_is_solid(
						chunk_data, iter, all_offsets[DOWN]),
				};

				block_t voxel = *terrain_voxel_data_get_const(chunk_data, iter);

				terrain_add_voxel_to_mesher(mesher, iter, chunk_data->coords,
											faces, chunk_data->uv_rect_lookup,
											voxel);
			}
		}
	}
}

void terrain_voxel_data_generate(IntermediateVoxelData* voxels)
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
				*terrain_voxel_data_get(voxels, iter) =
					terrain_generate_voxel(voxels->coords, iter);
			}
		}
	}
}

block_t* terrain_voxel_data_get(IntermediateVoxelData* voxels,
								VoxelCoords voxel)
{
	assert(voxel.z + voxel.x * max_voxelcoord.x + voxel.y * max_voxelcoord.y <
		   CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT);
	return &voxels->voxels[voxel.z + voxel.x * max_voxelcoord.x +
						   voxel.y * max_voxelcoord.y];
}

const block_t* terrain_voxel_data_get_const(const IntermediateVoxelData* voxels,
											VoxelCoords voxel)
{
	const size_t val =
		voxel.z + voxel.x * max_voxelcoord.x + voxel.y * max_voxelcoord.y;
	static const size_t max = (long)CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT;
	assert(val < max);
	return &voxels->voxels[val];
}

size_t terrain_voxel_data_get_face_count(const IntermediateVoxelData* voxels)
{
	size_t count = 0;
	VoxelCoords iter = {0};
	for (; iter.x < max_voxelcoord.x; ++iter.x) {
		for (iter.y = 0; iter.y < max_voxelcoord.y; ++iter.y) {
			for (iter.z = 0; iter.z < max_voxelcoord.z; ++iter.z) {
				const block_t* voxel =
					terrain_voxel_data_get_const(voxels, iter);

				if (!terrain_voxel_is_solid(*voxel)) {
					// empty
					continue;
				}

				// otherwise we need to count the number of solid neighbors
				for (uint8_t i = 0; i < NUM_SIDES; ++i) {
					// if the neighbor isn't solid, that means the  face is
					// exposed and needs to be rendered
					if (!terrain_voxel_data_get_offset_is_solid(
							voxels, iter, all_offsets[i])) {
						++count;
					}
				}
			}
		}
	}

	return count;
}

static bool
terrain_voxel_data_get_offset_is_solid(const IntermediateVoxelData* chunk_data,
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
		// if overflowing off the top, generate a face, otherwise don't. so that
		// players looking down on the terrain always have faces to see
		if (offset.negative && coords.y == 0) {
			return true;
		} else if ((!offset.negative && coords.y == max_voxelcoord.y - 1)) {
			return false;
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
		*terrain_voxel_data_get_const(chunk_data, offset_coords);
	return terrain_voxel_is_solid(voxel);
}
