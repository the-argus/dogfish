#include "input.h"
#include "raylib.h"
#include "raymath.h"
#include "architecture.h"
#include "constants.h"

void set_virtual_mouse_position(struct Gamestate *gamestate,
								float screen_scale_fraction)
{
	// get the mouse position as if the window was not scaled from render size
	gamestate->input.mouse.virtual_position.x =
		(gamestate->input.mouse.position.x -
		 (GetScreenWidth() - (GAME_WIDTH * screen_scale_fraction)) * 0.5f) /
		screen_scale_fraction;
	gamestate->input.mouse.virtual_position.y =
		(gamestate->input.mouse.position.y -
		 (GetScreenHeight() - (GAME_HEIGHT * screen_scale_fraction)) * 0.5f) /
		screen_scale_fraction;

	// clamp the virtual mouse position to the size of the rendertarget
	gamestate->input.mouse.virtual_position =
		Vector2Clamp(gamestate->input.mouse.virtual_position, (Vector2){0, 0},
					 (Vector2){(float)GAME_WIDTH, (float)GAME_HEIGHT});
}

float get_joystick(ControllerState *cstate) { return (float)cstate->joystick; }

/// Make the gamestate reflect the actual system IO state.
void gather_input(Gamestate *gamestate, float screen_scaling)
{
	// collect mouse information
	gamestate->input.mouse.position = GetMousePosition();
	set_virtual_mouse_position(gamestate, screen_scaling);

	gamestate->input.mouse.left_pressed =
		IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	gamestate->input.mouse.right_pressed =
		IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

	// collect keyboard information
	gamestate->input.keys.jump = IsKeyDown(KEY_SPACE);
	gamestate->input.keys.right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
	gamestate->input.keys.left = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
	gamestate->input.keys.up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
	gamestate->input.keys.down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
}

// return 1 if the controls to exit the game are being pressed, 0 otherwise
int exit_control_pressed()
{
	return IsKeyPressed(KEY_ESCAPE) ||
		   (IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT) &&
			IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
}
