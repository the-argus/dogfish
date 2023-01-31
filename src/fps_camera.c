#include "fps_camera.h"
#include "constants.h"
#include "raylib.h"
#include <math.h>

typedef enum
{
	MOVE_FRONT = 0,
	MOVE_BACK,
	MOVE_RIGHT,
	MOVE_LEFT,
	MOVE_UP,
	MOVE_DOWN
} CameraMove;

#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef DEG2RAD
#define DEG2RAD (PI / 180.0f)
#endif
// unused atm
#ifndef RAD2DEG
#define RAD2DEG (180.0f / PI)
#endif

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

// PLAYER (used by camera)
#define PLAYER_MOVEMENT_SENSITIVITY 2.0f

CameraData camera_data = {
	// Global CAMERA state context
	.targetDistance = 0,
	.playerEyesPosition = PLAYER_EYE_HEIGHT,
	.angle = {0},
	.moveControl = CAMERA_CONTROLS,
};

void FpsCameraUpdate(Camera *camera)
{
	static float swingCounter = 0.0f; // Used for 1st person swinging movement

	// TODO: Compute CAMERA.targetDistance and CAMERA.angle here (?)

	// Mouse movement detection
	Vector2 mousePositionDelta = GetMouseDelta();

	// Keys input detection
	// TODO: Input detection is raylib-dependant, it could be moved outside the
	// module
	bool direction[6] = {IsKeyDown(camera_data.moveControl[MOVE_FRONT]),
						 IsKeyDown(camera_data.moveControl[MOVE_BACK]),
						 IsKeyDown(camera_data.moveControl[MOVE_RIGHT]),
						 IsKeyDown(camera_data.moveControl[MOVE_LEFT]),
						 IsKeyDown(camera_data.moveControl[MOVE_UP]),
						 IsKeyDown(camera_data.moveControl[MOVE_DOWN])};
	camera->position.x += (sinf(camera_data.angle.x) * direction[MOVE_BACK] -
						   sinf(camera_data.angle.x) * direction[MOVE_FRONT] -
						   cosf(camera_data.angle.x) * direction[MOVE_LEFT] +
						   cosf(camera_data.angle.x) * direction[MOVE_RIGHT]) *
						  PLAYER_MOVEMENT_SENSITIVITY * GetFrameTime();

	camera->position.y +=
		(sinf(camera_data.angle.y) * direction[MOVE_FRONT] -
		 sinf(camera_data.angle.y) * direction[MOVE_BACK] +
		 1.0f * direction[MOVE_UP] - 1.0f * direction[MOVE_DOWN]) *
		PLAYER_MOVEMENT_SENSITIVITY * GetFrameTime();

	camera->position.z += (cosf(camera_data.angle.x) * direction[MOVE_BACK] -
						   cosf(camera_data.angle.x) * direction[MOVE_FRONT] +
						   sinf(camera_data.angle.x) * direction[MOVE_LEFT] -
						   sinf(camera_data.angle.x) * direction[MOVE_RIGHT]) *
						  PLAYER_MOVEMENT_SENSITIVITY * GetFrameTime();

	// Camera orientation calculation
	camera_data.angle.x -=
		mousePositionDelta.x * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();
	camera_data.angle.y -=
		mousePositionDelta.y * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();

	// Angle clamp
	if (camera_data.angle.y > CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD)
		camera_data.angle.y = CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD;
	else if (camera_data.angle.y < CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD)
		camera_data.angle.y = CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD;

	// Calculate translation matrix
	Matrix matTranslation = {1.0f, 0.0f, 0.0f, 0.0f,
							 0.0f, 1.0f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 1,
							 0.0f, 0.0f, 0.0f, 1.0f};

	// Calculate rotation matrix
	Matrix matRotation = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

	float cosz = cosf(0.0f);
	float sinz = sinf(0.0f);
	float cosy = cosf(-(PI * 2 - camera_data.angle.x));
	float siny = sinf(-(PI * 2 - camera_data.angle.x));
	float cosx = cosf(-(PI * 2 - camera_data.angle.y));
	float sinx = sinf(-(PI * 2 - camera_data.angle.y));

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
	Matrix matTransform = {0};
	matTransform.m0 = matTranslation.m0 * matRotation.m0 +
					  matTranslation.m1 * matRotation.m4 +
					  matTranslation.m2 * matRotation.m8 +
					  matTranslation.m3 * matRotation.m12;
	matTransform.m1 = matTranslation.m0 * matRotation.m1 +
					  matTranslation.m1 * matRotation.m5 +
					  matTranslation.m2 * matRotation.m9 +
					  matTranslation.m3 * matRotation.m13;
	matTransform.m2 = matTranslation.m0 * matRotation.m2 +
					  matTranslation.m1 * matRotation.m6 +
					  matTranslation.m2 * matRotation.m10 +
					  matTranslation.m3 * matRotation.m14;
	matTransform.m3 = matTranslation.m0 * matRotation.m3 +
					  matTranslation.m1 * matRotation.m7 +
					  matTranslation.m2 * matRotation.m11 +
					  matTranslation.m3 * matRotation.m15;
	matTransform.m4 = matTranslation.m4 * matRotation.m0 +
					  matTranslation.m5 * matRotation.m4 +
					  matTranslation.m6 * matRotation.m8 +
					  matTranslation.m7 * matRotation.m12;
	matTransform.m5 = matTranslation.m4 * matRotation.m1 +
					  matTranslation.m5 * matRotation.m5 +
					  matTranslation.m6 * matRotation.m9 +
					  matTranslation.m7 * matRotation.m13;
	matTransform.m6 = matTranslation.m4 * matRotation.m2 +
					  matTranslation.m5 * matRotation.m6 +
					  matTranslation.m6 * matRotation.m10 +
					  matTranslation.m7 * matRotation.m14;
	matTransform.m7 = matTranslation.m4 * matRotation.m3 +
					  matTranslation.m5 * matRotation.m7 +
					  matTranslation.m6 * matRotation.m11 +
					  matTranslation.m7 * matRotation.m15;
	matTransform.m8 = matTranslation.m8 * matRotation.m0 +
					  matTranslation.m9 * matRotation.m4 +
					  matTranslation.m10 * matRotation.m8 +
					  matTranslation.m11 * matRotation.m12;
	matTransform.m9 = matTranslation.m8 * matRotation.m1 +
					  matTranslation.m9 * matRotation.m5 +
					  matTranslation.m10 * matRotation.m9 +
					  matTranslation.m11 * matRotation.m13;
	matTransform.m10 = matTranslation.m8 * matRotation.m2 +
					   matTranslation.m9 * matRotation.m6 +
					   matTranslation.m10 * matRotation.m10 +
					   matTranslation.m11 * matRotation.m14;
	matTransform.m11 = matTranslation.m8 * matRotation.m3 +
					   matTranslation.m9 * matRotation.m7 +
					   matTranslation.m10 * matRotation.m11 +
					   matTranslation.m11 * matRotation.m15;
	matTransform.m12 = matTranslation.m12 * matRotation.m0 +
					   matTranslation.m13 * matRotation.m4 +
					   matTranslation.m14 * matRotation.m8 +
					   matTranslation.m15 * matRotation.m12;
	matTransform.m13 = matTranslation.m12 * matRotation.m1 +
					   matTranslation.m13 * matRotation.m5 +
					   matTranslation.m14 * matRotation.m9 +
					   matTranslation.m15 * matRotation.m13;
	matTransform.m14 = matTranslation.m12 * matRotation.m2 +
					   matTranslation.m13 * matRotation.m6 +
					   matTranslation.m14 * matRotation.m10 +
					   matTranslation.m15 * matRotation.m14;
	matTransform.m15 = matTranslation.m12 * matRotation.m3 +
					   matTranslation.m13 * matRotation.m7 +
					   matTranslation.m14 * matRotation.m11 +
					   matTranslation.m15 * matRotation.m15;

	camera->target.x = camera->position.x - matTransform.m12;
	camera->target.y = camera->position.y - matTransform.m13;
	camera->target.z = camera->position.z - matTransform.m14;

	// Camera position update
	// NOTE: On CAMERA_FIRST_PERSON player Y-movement is limited to player 'eyes
	// position'
	camera->position.y = camera_data.playerEyesPosition;

	// Camera swinging (y-movement), only when walking (some key pressed)
	for (int i = 0; i < 6; i++)
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
