#pragma once
#include "object_structure.h"
#include <raylib.h>

// TODO: add struct __aligned__ attrs for better access speed, worse memory
// usage. maybe more cache misses? do profiling. see clang-tidy
// altera-struct-pack-align

typedef struct
{
	Camera3D camera;
	Vector2 angle; // Camera angle in plane XZ
} FullCamera;

typedef struct Cursorstate
{
	Vector2 position;
	Vector2 virtual_position;
	Vector2 delta;
	uint8_t shoot : 1;
} Cursorstate;

typedef struct Keystate
{
	uint8_t up : 1;
	uint8_t left : 1;
	uint8_t right : 1;
	uint8_t down : 1;
	uint8_t boost : 1;
} Keystate;

typedef Vector2 Joystick;

typedef struct ControllerState
{
	uint8_t boost : 1;
	uint8_t shoot : 1;
	Joystick joystick;
} ControllerState;

typedef struct Inputstate
{
	Cursorstate cursor;
	Cursorstate cursor_2;
	Keystate keys;
	Keystate keys_2;
	ControllerState controller;
	ControllerState controller_2;
} Inputstate;
