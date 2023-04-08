#pragma once

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 480
// the size at which the game is rendered
#define GAME_WIDTH 720
#define GAME_HEIGHT 480

#define FOV 90

// ../assets because the executable is in the build directory usually
#ifndef ASSETS_FOLDER
#define ASSETS_FOLDER "assets"
#endif

// FPS camera related constants
typedef enum
{
	MOVE_FRONT = 0,
	MOVE_BACK,
	MOVE_RIGHT,
	MOVE_LEFT,
	MOVE_UP,
	MOVE_DOWN
} MovementKeys;

#define PLAYER_MOVE_IMPULSE 1
#define PLAYER_JUMP_FORCE 30

// how up-facing a collision must be to count as the "ground". 1 is perfectly up
// and 0 is any collision
#define ON_GROUND_THRESHHOLD 0.7f

// physics constants
#define GRAVITY 0.5
