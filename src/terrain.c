#include "terrain.h"
#include "raymath.h"

#define _IMPLEMENT_DYNARRAY
#define DYNARRAY_TYPE Vector3
#define DYNARRAY_TYPE_NAME Dynarray_Vector3
#include "dynarray.h"

static const int size = 50;			  // size of terrain cube
static const float threshhold = 0.5f; // value below which perlin = air
static const float scale = 1;
#define NOISE_SETTINGS 2, 10, 3

static Mesh cube;
static Material terrain_material;
static Shader terrain_shader;
static Matrix *transforms;
static uint transforms_size;

static float perlin(float x, float y, float z, float gain, int octaves,
					int hgrid);

void load_terrain()
{
	Dynarray_Vector3 terrain_nodes = dynarray_create_Vector3(100);
	cube = GenMeshCube(scale, scale, scale);

	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			for (int z = 0; z < size; z++) {
				uchar this = perlin(x, y, z, NOISE_SETTINGS) > threshhold;
				uchar adj_1 = perlin(x + 1, y, z, NOISE_SETTINGS) > threshhold;
				uchar adj_2 = perlin(x - 1, y, z, NOISE_SETTINGS) > threshhold;
				uchar adj_3 = perlin(x, y + 1, z, NOISE_SETTINGS) > threshhold;
				uchar adj_4 = perlin(x, y - 1, z, NOISE_SETTINGS) > threshhold;
				uchar adj_5 = perlin(x, y, z + 1, NOISE_SETTINGS) > threshhold;
				uchar adj_6 = perlin(x, y, z - 1, NOISE_SETTINGS) > threshhold;
				uchar surrounded =
					adj_1 && adj_2 && adj_3 && adj_4 && adj_5 && adj_6;

				if (this && !surrounded) {
					Vector3 node = {x * scale, y * scale, z * scale};
					dynarray_insert_Vector3(&terrain_nodes, node);
				}
			}
		}
	}

	// convert terrain nodes into transforms to be uploaded to GPU for instances
	transforms = (Matrix *)RL_CALLOC(
		terrain_nodes.size, // maximum possible instances
		sizeof(Matrix));	// Pre-multiplied transformations passed to rlgl
	transforms_size = terrain_nodes.size;

	for (uint i = 0; i < terrain_nodes.size; i++) {
		transforms[i] =
			MatrixTranslate(terrain_nodes.head[i].x, terrain_nodes.head[i].y,
							terrain_nodes.head[i].z);
	}

	// initialize terrain material
	terrain_shader = LoadShader("assets/materials/instanced.vs", "assets/materials/instanced.fs");
	// Get shader locations
	terrain_shader.locs[SHADER_LOC_MATRIX_MVP] =
		GetShaderLocation(terrain_shader, "mvp");
	terrain_shader.locs[SHADER_LOC_MATRIX_MODEL] =
		GetShaderLocationAttrib(terrain_shader, "instanceTransform");
	terrain_material = LoadMaterialDefault();
	terrain_material.shader = terrain_shader;
	terrain_material.maps[MATERIAL_MAP_DIFFUSE].color = GREEN;

	// cleanup dynamic array
	free(terrain_nodes.head);
}

void draw_terrain()
{
	DrawMeshInstanced(cube, terrain_material, transforms, transforms_size);
}

void cleanup_terrain()
{
	RL_FREE(transforms);
	UnloadShader(terrain_shader);
}

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
static float perlin(float x, float y, float z, float gain, int octaves,
					int hgrid)
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
