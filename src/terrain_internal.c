#include "terrain_internal.h"
#include "threadutils.h"
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

	assert(coords.x >= 0 && coords.x < CHUNK_SIZE);
	assert(coords.y >= 0 && coords.y < WORLD_HEIGHT);
	assert(coords.y >= 0 && coords.z < CHUNK_SIZE);

	return coords;
}
