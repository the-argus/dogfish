#include "terrain.h"

// Model create_terrain(int width, int height, float cube_size) {}

///
/// Generate a random value given three integers (intended to be coordinates)
/// Also adapted from
/// https://stackoverflow.com/questions/16569660/2d-perlin-noise-in-c
///
static float noise_3d(float x, float y, float z)
{
	int n;

	n = x + (y * 57) + (z * 373);
	n = (n << 13) ^ n;
	return (1.0 - ((n * ((n * n * 15731) + 789221) + 1376312589) & 0x7fffffff) /
					  1073741824.0);
}

///
/// Retrieve a value of perlin noise in 3d.
/// Amplitude of output is equal to gain^octaves.
///
/// Adapted from code found here:
/// https://stackoverflow.com/questions/16569660/2d-perlin-noise-in-c
///
/// @param x: x coord
/// @param y: y coord
/// @param z: z coord
/// @param gain: constant by which noise is scaled each octave.
/// @param octaves: number of noise iterations. increasing this drastically
/// increases the amplitude of the final noise.
/// @param hgrid: the inverse of the initial frequency. Must be non-zero.
/// smaller hgrid means the smallest noise will be more detailed, and... TODO:
/// figure out what hgrid actually does besides that lel
///
float perlin(float x, float y, float z, float gain, int octaves, int hgrid)
{
	int i;
	float total = 0.0f;
	float frequency = 1.0f / (float)hgrid;
	float amplitude = gain;
	float lacunarity = 2.0; // basically frequency multiplier

	for (i = 0; i < octaves; i++) {
		total += noise_3d((float)x * frequency, (float)y * frequency,
						  (float)z * frequency) *
				 amplitude;
		frequency *= lacunarity;
		amplitude *= gain;
	}

	return total;
}
