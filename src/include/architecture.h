#pragma once
#include "raylib.h"

typedef struct
{
	Vector2 angle;			  // Camera angle in plane XZ
	int moveControl[6];	   // Move controls (CAMERA_FIRST_PERSON)
} CameraData;

typedef struct Mousestate {
    unsigned char left_pressed;
    unsigned char right_pressed;
    Vector2 position;
    Vector2 virtual_position;
} Mousestate;

typedef struct Keystate {
    unsigned char up;
    unsigned char left;
    unsigned char right;
    unsigned char down;
    unsigned char jump;
} Keystate;

typedef struct Inputstate {
    Mousestate mouse;
    Keystate keys;
} Inputstate;

typedef struct Gamestate {
    Inputstate input;
    Camera *current_camera;
    CameraData camera_data;
} Gamestate;
