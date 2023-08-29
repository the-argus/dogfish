#include "terrain_internal.h"

float perlin_3d(float x, float y, float z, const NoiseOptions* options)
{
	float total = 0.0f;
	float frequency = 1.0f / (float)options->hgrid;
	float amplitude = options->gain;
	float lacunarity = 2.0; // basically frequency multiplier

	for (size_t i = 0; i < options->octaves; i++) {
		total +=
			noise_3d(x * frequency, y * frequency, z * frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= options->gain;
	}

	return total;
}

float perlin_2d(float x, float y, const NoiseOptions* options)
{
	float total = 0.0f;
	float frequency = 1.0f / (float)options->hgrid;
	float amplitude = options->gain;
	float lacunarity = 2.0f; // basically frequency multiplier

	for (size_t i = 0; i < options->octaves; i++) {
		total += noise_2d(x * frequency, y * frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= options->gain;
	}

	return total;
}

float noise_3d(float x, float y, float z)
{
	int n = 0;

	n = x + (y * 57);
	float nf = *((float*)&n);
	nf *= 373 * z;
	n = *(int*)&nf;
	n = ((*(uint32_t*)&n) << 13) ^ n;
	return (float)(1.0 - ((((*(uint32_t*)&n) *
								((*(uint32_t*)&n) * (*(uint32_t*)&n) * 15731) +
							789221) +
						   1376312589) &
						  0x7fffffff) /
							 1073741824.0);
}

float noise_2d(float x, float y)
{
	int n;

	n = x + (y * 57);
	n = (n << 13) ^ n;
	return (float)(1.0 - ((n * ((n * n * 15731) + 789221) + 1376312589) &
						  0x7fffffff) /
							 1073741824.0);
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
