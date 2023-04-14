#include "raylib.h"
#include "raymath.h"

#include "terrain.h"

#define _IMPLEMENT_DYNARRAY
#undef DYNARRAY_TYPE
#undef DYNARRAY_TYPE_NAME
#define DYNARRAY_TYPE Vector3
#define DYNARRAY_TYPE_NAME Dynarray_Vector3
#include "dynarray.h"

static const int size = 50;			  // size of terrain cube
static const float threshhold = 6.0f; // value below which perlin = air
static const float scale = 1;
#define NOISE_SETTINGS 10, 3, 1
#define NOISE_OFFSET 12

static Mesh cube;
static Material terrain_material;
static Shader terrain_shader;
static Matrix *transforms;
static dGeomID *geoms;
static uint transforms_size;

static float perlin_3d(float x, float y, float z, float gain, int octaves,
					   int hgrid);
static float perlin_2d(float x, float y, float gain, int octaves, int hgrid);

void load_terrain(dSpaceID space)
{
	Dynarray_Vector3 terrain_nodes = dynarray_create_Vector3(100);
	cube = GenMeshCube(scale, scale, scale);

	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			for (int z = 0; z < size; z++) {
				float this_perlin =
					perlin_3d(x, y, z, NOISE_SETTINGS) + NOISE_OFFSET;
				uchar this = this_perlin > threshhold;
				uchar adj_1 =
					perlin_3d(x + 1, y, z, NOISE_SETTINGS) + NOISE_OFFSET >
					threshhold;
				uchar adj_2 =
					perlin_3d(x - 1, y, z, NOISE_SETTINGS) + NOISE_OFFSET >
					threshhold;
				uchar adj_3 =
					perlin_3d(x, y + 1, z, NOISE_SETTINGS) + NOISE_OFFSET >
					threshhold;
				uchar adj_4 =
					perlin_3d(x, y - 1, z, NOISE_SETTINGS) + NOISE_OFFSET >
					threshhold;
				uchar adj_5 =
					perlin_3d(x, y, z + 1, NOISE_SETTINGS) + NOISE_OFFSET >
					threshhold;
				uchar adj_6 =
					perlin_3d(x, y, z - 1, NOISE_SETTINGS) + NOISE_OFFSET >
					threshhold;
				uchar surrounded =
					adj_1 && adj_2 && adj_3 && adj_4 && adj_5 && adj_6;

				float groundh = perlin_2d(x, z, 5, 1, 1) + 12;
				uchar ground = y < groundh;
				// printf("%f\n", this_perlin);

				if (this && !surrounded && ground) {
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

	// allocate space for physics geometry pointers
	geoms = malloc(sizeof(dGeomID) * terrain_nodes.size);
	if (geoms == NULL) {
		printf("Memory allocation failure for terrain physics geometries\n");
		exit(EXIT_FAILURE);
	}
	assert(space != NULL);

	for (uint i = 0; i < terrain_nodes.size; i++) {
		// create physics geometries
		dGeomID geom = dCreateBox(space, scale, scale, scale);
		dGeomSetPosition(geom, terrain_nodes.head[i].x * scale,
						 terrain_nodes.head[i].y * scale,
						 terrain_nodes.head[i].z * scale);
		geoms[i] = geom;
		// create list of transforms
		transforms[i] = MatrixTranslate(terrain_nodes.head[i].x * scale,
										terrain_nodes.head[i].y * scale,
										terrain_nodes.head[i].z * scale);
	}

	// initialize terrain material
	terrain_shader = LoadShader("assets/materials/instanced.vs",
								"assets/materials/instanced.fs");
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
	free(geoms);
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

	n = x + (y * 57);
	float nf = *((float *)&n);
	nf *= 373 * z;
	n = *(int *)&nf;
	n = (n << 13) ^ n;
	return (1.0 - ((n * ((n * n * 15731) + 789221) + 1376312589) & 0x7fffffff) /
					  1073741824.0);
}

static float noise_2d(float x, float y)
{
	int n;

	n = x + (y * 57);
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
static float perlin_3d(float x, float y, float z, float gain, int octaves,
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

static float perlin_2d(float x, float y, float gain, int octaves, int hgrid)
{
	int i;
	float total = 0.0f;
	float frequency = 1.0f / (float)hgrid;
	float amplitude = gain;
	float lacunarity = 2.0; // basically frequency multiplier

	for (i = 0; i < octaves; i++) {
		total +=
			noise_2d((float)x * frequency, (float)y * frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= gain;
	}

	return total;
}
