#include "raylib.h"
#include "architecture.h"
#include "constants.h"
#include "fps_camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// I have to just remember to free the camera that gets malloced here :(
void gamestate_new_fps_camera(struct Gamestate *gamestate)
{
	Camera3D *camera;
	camera = malloc(sizeof(Camera3D));
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
		.targetDistance = 0,
		.playerEyesPosition = camera->position.y,
		.angle = {0},
		.moveControl = CAMERA_CONTROLS,
	};

	Vector3 v1 = camera->position;
	Vector3 v2 = camera->target;

	float dx = v2.x - v1.x;
	float dy = v2.y - v1.y;
	float dz = v2.z - v1.z;

	// Distance to target
	camera_data.targetDistance = sqrtf(dx * dx + dy * dy + dz * dz);

	// Camera angle calculation
	// Camera angle in plane XZ (0 aligned with Z, move positive CCW)
	camera_data.angle.x = atan2f(dx, dz);
	// Camera angle in plane XY (0 aligned with X, move positive CW)
	camera_data.angle.y = atan2f(dy, sqrtf(dx * dx + dz * dz));

	// Lock cursor for first person and third person cameras
	DisableCursor();

	gamestate->current_camera = camera;
	gamestate->camera_data = camera_data;
}
