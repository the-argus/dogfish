#include "raylib.h"
#include "architecture.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>

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

    gamestate->current_camera = camera;
}
