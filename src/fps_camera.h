#pragma once
#include "raylib.h"

typedef struct
{
	Vector2 angle;			  // Camera angle in plane XZ

	// Camera movement control keys
	int moveControl[6];	   // Move controls (CAMERA_FIRST_PERSON)
} CameraData;

void fps_camera_update(Camera *camera, CameraData *camera_data);
void update_camera_tilt(Camera *camera, CameraData *camera_data);
