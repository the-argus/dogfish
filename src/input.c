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

	// player 1 moves with WASD
	gamestate->input.keys.right = IsKeyDown(KEY_D);
	gamestate->input.keys.left = IsKeyDown(KEY_A);
	gamestate->input.keys.up = IsKeyDown(KEY_W);
	gamestate->input.keys.down = IsKeyDown(KEY_S);

	gamestate->input.keys.jump = IsKeyDown(KEY_Z);

	// player 2 moves with arrow keys
	gamestate->input.keys_2.right = IsKeyDown(KEY_RIGHT);
	gamestate->input.keys_2.left = IsKeyDown(KEY_LEFT);
	gamestate->input.keys_2.up = IsKeyDown(KEY_UP);
	gamestate->input.keys_2.down = IsKeyDown(KEY_DOWN);

	gamestate->input.keys_2.jump = IsKeyDown(KEY_SPACE);

	// collect controller information

	// player 1 uses d-pad and left joystick
	gamestate->input.controller.down =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
	gamestate->input.controller.up =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP);
	gamestate->input.controller.left =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
	gamestate->input.controller.right =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

	gamestate->input.controller.joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
	gamestate->input.controller.joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

	// player 2 uses buttons (X Y A B) and right joystick
	gamestate->input.controller_2.down =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
	gamestate->input.controller_2.up =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
	gamestate->input.controller_2.left =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
	gamestate->input.controller_2.right =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

	gamestate->input.controller_2.joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
	gamestate->input.controller_2.joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
}

// return 1 if the controls to exit the game are being pressed, 0 otherwise
int exit_control_pressed()
{
	return IsKeyPressed(KEY_ESCAPE) ||
		   (IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT) &&
			IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
}
