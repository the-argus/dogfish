#pragma once
#include <raylib.h>

typedef struct
{
	Camera3D camera;
	Vector2 angle; // Camera angle in plane XZ
} FullCamera;

void camera_new(FullCamera* camera_to_construct);
