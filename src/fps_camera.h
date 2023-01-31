#pragma once
#include "raylib.h"

typedef struct
{
	float targetDistance;	  // Camera distance from position to target
	float playerEyesPosition; // Player eyes position from ground (in meters)
	Vector2 angle;			  // Camera angle in plane XZ

	// Camera movement control keys
	int moveControl[6];	   // Move controls (CAMERA_FIRST_PERSON)
} CameraData;

void FpsCameraUpdate(Camera *camera, CameraData *camera_data);
