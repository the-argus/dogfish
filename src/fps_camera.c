#include "fps_camera.h"
#include "constants.h"
#include "raylib.h"
#include "raymath.h"
#include "architecture.h"
#include <math.h>
#include <stdio.h>

// Camera mouse movement sensitivity
// TODO: it should be independent of framerate
#define CAMERA_MOUSE_MOVE_SENSITIVITY 0.5f

// FIRST_PERSON
//#define CAMERA_FIRST_PERSON_MOUSE_SENSITIVITY           0.003f
#define CAMERA_FIRST_PERSON_FOCUS_DISTANCE 25.0f
#define CAMERA_FIRST_PERSON_MIN_CLAMP 89.0f
#define CAMERA_FIRST_PERSON_MAX_CLAMP -89.0f

// When walking, y-position of the player moves up-down at step frequency
// (swinging) but also the body slightly tilts left-right on every step, when
// all the body weight is left over one foot (tilting)

// Step frequency when walking (steps per second)
#define CAMERA_FIRST_PERSON_STEP_FREQUENCY 1.8f
// Maximum up-down swinging distance when walking
#define CAMERA_FIRST_PERSON_SWINGING_DELTA 0.03f
// Maximum left-right tilting distance when walking
#define CAMERA_FIRST_PERSON_TILTING_DELTA 0.005f

void fps_camera_update(Camera *camera, CameraData *camera_data)
{
	// Mouse movement detection
	Vector2 mousePositionDelta = GetMouseDelta();

	// Camera orientation calculation
	camera_data->angle.x -=
		mousePositionDelta.x * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();
	camera_data->angle.y -=
		mousePositionDelta.y * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();

	// Angle clamp
	if (camera_data->angle.y > CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD)
		camera_data->angle.y = CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD;
	else if (camera_data->angle.y < CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD)
		camera_data->angle.y = CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD;

	// clamp X
	camera_data->angle.x -= ((int)(camera_data->angle.x / (2 * PI))) * (2 * PI);

	// Calculate translation matrix
	// clang-format off
	Matrix matTranslation = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    Matrix matRotation = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
	// clang-format on

	float cosz = cosf(0.0f);
	float sinz = sinf(0.0f);
	float cosy = cosf(-(PI * 2 - camera_data->angle.x));
	float siny = sinf(-(PI * 2 - camera_data->angle.x));
	float cosx = cosf(-(PI * 2 - camera_data->angle.y));
	float sinx = sinf(-(PI * 2 - camera_data->angle.y));

	matRotation.m0 = cosz * cosy;
	matRotation.m4 = (cosz * siny * sinx) - (sinz * cosx);
	matRotation.m8 = (cosz * siny * cosx) + (sinz * sinx);
	matRotation.m1 = sinz * cosy;
	matRotation.m5 = (sinz * siny * sinx) + (cosz * cosx);
	matRotation.m9 = (sinz * siny * cosx) - (cosz * sinx);
	matRotation.m2 = -siny;
	matRotation.m6 = cosy * sinx;
	matRotation.m10 = cosy * cosx;

	// Multiply translation and rotation matrices
	Matrix matTransform = MatrixMultiply(matTranslation, matRotation);

	camera->target.x = camera->position.x - matTransform.m12;
	camera->target.y = camera->position.y - matTransform.m13;
	camera->target.z = camera->position.z - matTransform.m14;
}

void update_camera_tilt(Camera *camera, Inputstate input)
{
	static float swingCounter = 0.0f; // Used for 1st person swinging movement

	bool direction[4] = {input.keys.up, input.keys.down, input.keys.left,
						 input.keys.right};

	// Camera swinging (y-movement), only when walking (some key pressed)
	for (int i = 0; i < 4; i++)
		if (direction[i]) {
			swingCounter += GetFrameTime();
			break;
		}

	camera->position.y -=
		sinf(2 * PI * CAMERA_FIRST_PERSON_STEP_FREQUENCY * swingCounter) *
		CAMERA_FIRST_PERSON_SWINGING_DELTA;

	// Camera waiving (xz-movement), only when walking (some key pressed)
	camera->up.x =
		sinf(2 * PI * CAMERA_FIRST_PERSON_STEP_FREQUENCY * swingCounter) *
		CAMERA_FIRST_PERSON_TILTING_DELTA;
	camera->up.z =
		-sinf(2 * PI * CAMERA_FIRST_PERSON_STEP_FREQUENCY * swingCounter) *
		CAMERA_FIRST_PERSON_TILTING_DELTA;
}
