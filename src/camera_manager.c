#include "raylib.h"
#include "architecture.h"
#include "constants.h"

void gamestate_new_fps_camera(struct Gamestate *gamestate)
{
	// 3D camera
	Camera3D camera = {0};
	camera.position = (Vector3){0.0f, 10.0f, 10.0f};
	camera.target = (Vector3){0.0f, 0.0f, 0.0f};
	camera.up = (Vector3){0.0f, 1.0f, 0.0f};
	camera.fovy = (float)FOV;
	camera.projection = CAMERA_PERSPECTIVE;
}
