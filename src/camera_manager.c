#include "architecture.h"
#include "constants.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

// I have to just remember to free the camera that gets malloced here :(
// player_index is either 0 (p1) or 1 (p2)
void gamestate_new_fps_camera(struct Gamestate *gamestate, unsigned int player_index)
{
	Camera3D *camera = malloc(sizeof(Camera3D));
	if (camera == NULL) {
		printf("Malloc for Camera3D failed\n");
		return;
	}
	camera->position = (Vector3){0.0f, 10.0f, 10.0f};
	camera->target = (Vector3){0.0f, 0.0f, 0.0f};
	camera->up = (Vector3){0.0f, 1.0f, 0.0f};
	camera->fovy = (float)FOV;
	camera->projection = CAMERA_PERSPECTIVE;

	CameraData camera_data = {
		// Global CAMERA state context
		.angle = {0},
	};

	Vector3 v1 = camera->position;
	Vector3 v2 = camera->target;

	float dx = v2.x - v1.x;
	float dy = v2.y - v1.y;
	float dz = v2.z - v1.z;

	// Camera angle calculation
	// Camera angle in plane XZ (0 aligned with Z, move positive CCW)
	camera_data.angle.x = atan2f(dx, dz);
	// Camera angle in plane XY (0 aligned with X, move positive CW)
	camera_data.angle.y = atan2f(dy, sqrtf(dx * dx + dz * dz));

	// Lock cursor for first person and third person cameras
	DisableCursor();

	if (player_index == 0)
	{
		gamestate->p1_camera = camera;
		gamestate->p1_camera_data = camera_data;
	}
	else if (player_index == 1)
	{
		gamestate->p2_camera = camera;
		gamestate->p2_camera_data = camera_data;
	}
}
