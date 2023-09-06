#include "terrain_internal.h"
#include "constants/general.h"
#include "mesher.h"
#include "rlights.h"
#include "terrain.h"
#include "terrain_voxel_data.h"
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
			terrain_voxel_data_generate(voxels);
			// voxels are now filled with the correct block_t values, meshing
			// time
			// TODO: make this stack allocated, at least in release mode
			Mesher* mesher = RL_MALLOC(sizeof(Mesher));
			mesher_create(mesher);
			// pass number of faces into "quads" argument of allocate, since all
			// the faces are quads (these are cubes)
			size_t faces = terrain_voxel_data_get_face_count(voxels);
			mesher_allocate(mesher, faces);
			terrain_voxel_data_populate_mesher(voxels, mesher);
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
