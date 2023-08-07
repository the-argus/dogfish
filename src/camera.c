#include "architecture.h"
#include "constants.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

/// player_index is either 0 (p1) or 1 (p2)
void camera_new(FullCamera* camera_to_construct)
{
	camera_to_construct->camera.position = (Vector3){0.0f, 10.0f, 10.0f};
	camera_to_construct->camera.target = (Vector3){0.0f, 0.0f, 0.0f};
	camera_to_construct->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
	camera_to_construct->camera.fovy = (float)FOV;
	camera_to_construct->camera.projection = CAMERA_PERSPECTIVE;

	Vector3 v1 = camera_to_construct->camera.position;
	Vector3 v2 = camera_to_construct->camera.target;

	float dx = v2.x - v1.x;
	float dy = v2.y - v1.y;
	float dz = v2.z - v1.z;

	camera_to_construct->angle = (Vector2){
		// Camera angle in plane XZ (0 aligned with Z, move positive
		// CCW)
		.x = atan2f(dx, dz),
		// Camera angle in plane XY (0 aligned with X, move positive CW)
		.y = atan2f(dy, sqrtf(dx * dx + dz * dz)),
	};
}
