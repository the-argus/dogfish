#pragma once
#include "raylib.h"
#include "ode/ode.h"
#include "shorthand.h"
#include "object_structure.h"

typedef struct
{
	Vector2 angle;			  // Camera angle in plane XZ
} CameraData;

typedef struct Cursorstate {
    Vector2 position;
    Vector2 virtual_position;
    Vector2 delta;
	uchar shoot : 1;
} Cursorstate;

typedef struct Keystate {
    uchar up : 1;
    uchar left : 1;
    uchar right : 1;
    uchar down : 1;
    uchar boost : 1;
} Keystate;

typedef Vector2 Joystick;

typedef struct ControllerState {
    uchar boost : 1;
	uchar shoot : 1;
    Joystick joystick;
} ControllerState;

typedef struct Inputstate {
    Cursorstate cursor;
    Cursorstate cursor_2;
    Keystate keys;
    Keystate keys_2;
    ControllerState controller;
    ControllerState controller_2;
} Inputstate;

typedef struct Gamestate {
    Inputstate input;
    Camera *p1_camera;
    CameraData p1_camera_data;
    Camera *p2_camera;
    CameraData p2_camera_data;
    dWorldID world;
    dSpaceID space;
    float screen_scale;
    Model *p1_model;
    Model *p2_model;
	Dynarray_GameObject *objects;
} Gamestate;
