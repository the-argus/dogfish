#pragma once
#include <raylib.h>
#include <stdint.h>

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
	Cursorstate cursor[2];
	Keystate keys[2];
	ControllerState controller[2];
} Inputstate;

void input_gather();

int exit_control_pressed();

/// Returns the total X/Y movement input for a given player. Values on both
/// axes are clamped between -1 and 1 inclusive.
Vector2 total_input(uint8_t player_index);

/// Returns the total X/Y cursor delta for a given player. Values are not
/// clamped.
Vector2 total_cursor(uint8_t player_index);
