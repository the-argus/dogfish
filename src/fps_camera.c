#include "fps_camera.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
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

void fps_camera_update(FullCamera* player, Cursorstate cursor)
{
	// Camera orientation calculation
	player->angle.x -=
		cursor.delta.x * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();
	player->angle.y -=
		cursor.delta.y * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();

	// Angle clamp
	if (player->angle.y > CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD) {
		player->angle.y = CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD;
	} else if (player->angle.y < CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD) {
		player->angle.y = CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD;
	}

	// clamp X
	player->angle.x -= ((int)(player->angle.x / (2 * PI))) * (2 * PI);

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
	float cosy = cosf(-(PI * 2 - player->angle.x));
	float siny = sinf(-(PI * 2 - player->angle.x));
	float cosx = cosf(-(PI * 2 - player->angle.y));
	float sinx = sinf(-(PI * 2 - player->angle.y));

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

	player->camera.target.x = player->camera.position.x - matTransform.m12;
	player->camera.target.y = player->camera.position.y - matTransform.m13;
	player->camera.target.z = player->camera.position.z - matTransform.m14;
}
