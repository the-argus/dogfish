#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "constants.h"

static Model skybox;

void draw_skybox()
{
	// We are inside the cube, we need to disable backface culling!
	rlDisableBackfaceCulling();
	rlDisableDepthMask();
	DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, WHITE);
	rlEnableBackfaceCulling();
	rlEnableDepthMask();
}

void load_skybox()
{
	// Load skybox model
	Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
	skybox = LoadModelFromMesh(cube);

	// Load skybox shader and set required locations
	// NOTE Some locations are automatically set at shader loading
	skybox.materials[0].shader = LoadShader(
		TextFormat("%s/skybox/common/shaders/skybox.vs", ASSETS_FOLDER),
		TextFormat("%s/skybox/common/shaders/skybox.fs", ASSETS_FOLDER));

	SetShaderValue(
		skybox.materials[0].shader,
		GetShaderLocation(skybox.materials[0].shader, "environmentMap"),
		(int[1]){MATERIAL_MAP_CUBEMAP}, SHADER_UNIFORM_INT);
	SetShaderValue(skybox.materials[0].shader,
				   GetShaderLocation(skybox.materials[0].shader, "doGamma"),
				   (int[1]){0}, SHADER_UNIFORM_INT);
	SetShaderValue(skybox.materials[0].shader,
				   GetShaderLocation(skybox.materials[0].shader, "vflipped"),
				   (int[1]){0}, SHADER_UNIFORM_INT);

	// Load cubemap shader and setup required shader locations
	Shader shdrCubemap = LoadShader(
		TextFormat("%s/skybox/common/shaders/cubemap.vs", ASSETS_FOLDER),
		TextFormat("%s/skybox/common/shaders/cubemap.fs", ASSETS_FOLDER));

	SetShaderValue(shdrCubemap,
				   GetShaderLocation(shdrCubemap, "equirectangularMap"),
				   (int[1]){0}, SHADER_UNIFORM_INT);

	Image img = LoadImage("assets/skybox/bluesky/working_example.png");
	skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(
		img, CUBEMAP_LAYOUT_AUTO_DETECT); // CUBEMAP_LAYOUT_PANORAMA
	UnloadImage(img);
}
