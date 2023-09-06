#pragma once
#include "terrain_internal.h"

/// A buffer of data determining the contents of a chunk. Used to store the
/// generated state of the chunk before converting it to a mesh.
typedef struct
{
	ChunkCoords coords;
	block_t voxels[CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT];
	/// UV Rect for the texture of a given block_t, on a texture sampler stored
	/// somewhere else
	const Rectangle* uv_rect_lookup;
	size_t uv_rect_lookup_capacity;
} IntermediateVoxelData;

/// Get a voxel from a bunch of voxel data
block_t* terrain_voxel_data_get(IntermediateVoxelData* voxels,
								VoxelCoords voxel);
const block_t* terrain_voxel_data_get_const(const IntermediateVoxelData* voxels,
											VoxelCoords voxel);
// fill voxels with block_ts based on perlin noise values
void terrain_voxel_data_generate(IntermediateVoxelData* voxels);

size_t terrain_voxel_data_get_face_count(const IntermediateVoxelData* voxels);

void terrain_voxel_data_populate_mesher(
	const IntermediateVoxelData* restrict chunk_data, Mesher* restrict mesher);
