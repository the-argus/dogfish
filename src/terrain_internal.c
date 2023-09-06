#include "terrain_internal.h"
#include <FastNoiseLite.h>

static fnl_state terrain_noise_perlin_main;

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

VoxelCoords
terrain_add_offset_to_voxel_coord(const VoxelCoords* restrict coords,
								  const VoxelOffset* restrict offset)
{
	assert(((voxel_index_signed_t)coords->x) + offset->x >= 0);
	assert(((voxel_index_signed_t)coords->y) + offset->y >= 0);
	assert(((voxel_index_signed_t)coords->z) + offset->z >= 0);
	return (VoxelCoords){
		.x = coords->x + offset->x,
		.y = coords->y + offset->y,
		.z = coords->z + offset->z,
	};
}
