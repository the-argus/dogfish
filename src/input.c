#include "input.h"
#include "raylib.h"
#include "raymath.h"
#include "architecture.h"
#include "constants.h"

/// Makes sure the location of the virtual cursor respects whatever the
/// cursor's actual position is
static void set_virtual_cursor_position(Cursorstate *cursor,
										float screen_scale_fraction)
{
	// get the mouse position as if the window was not scaled from render size
	cursor->virtual_position.x =
		(cursor->position.x -
		 (GetScreenWidth() - (GAME_WIDTH * screen_scale_fraction)) * 0.5f) /
		screen_scale_fraction;
	cursor->virtual_position.y =
		(cursor->position.y -
		 (GetScreenHeight() - (GAME_HEIGHT * screen_scale_fraction)) * 0.5f) /
		screen_scale_fraction;

	// clamp the virtual mouse position to the size of the rendertarget
	cursor->virtual_position =
		Vector2Clamp(cursor->virtual_position, (Vector2){0, 0},
					 (Vector2){(float)GAME_WIDTH, (float)GAME_HEIGHT});
}

/// Make the gamestate reflect the actual system IO state.
void gather_input(Gamestate *gamestate)
{
	// collect keyboard information

	// player 1 moves with WASD
	gamestate->input.keys.right = IsKeyDown(KEY_D);
	gamestate->input.keys.left = IsKeyDown(KEY_A);
	gamestate->input.keys.up = IsKeyDown(KEY_W);
	gamestate->input.keys.down = IsKeyDown(KEY_S);
	gamestate->input.keys.boost = IsKeyDown(KEY_SPACE);

	// player 2 moves with arrow keys
	gamestate->input.keys_2.right = IsKeyDown(KEY_RIGHT);
	gamestate->input.keys_2.left = IsKeyDown(KEY_LEFT);
	gamestate->input.keys_2.up = IsKeyDown(KEY_UP);
	gamestate->input.keys_2.down = IsKeyDown(KEY_DOWN);
	gamestate->input.keys_2.boost = IsKeyDown(KEY_ENTER);

	// collect controller information

	// player 1 uses d-pad and left joystick
	gamestate->input.controller.boost =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP);
	gamestate->input.controller.shoot =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

	gamestate->input.controller.joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
	gamestate->input.controller.joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

	// player 2 uses buttons (X Y A B) and right joystick
	gamestate->input.controller_2.boost =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
	gamestate->input.controller_2.shoot =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

	gamestate->input.controller_2.joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
	gamestate->input.controller_2.joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

	// collect cursor information (mouse input only affects player 1's cursor)
	// *delta* is how much the cursor should move this frame. used by the first
	// person cameras to determine how much to rotate.
	gamestate->input.cursor.delta =
		Vector2Add(GetMouseDelta(), gamestate->input.controller.joystick);
	gamestate->input.cursor_2.delta = gamestate->input.controller_2.joystick;

	// *position* is where the cursor is on the screen. will be useful for
	// things like pause menus.
	gamestate->input.cursor.position = GetMousePosition();

	gamestate->input.cursor.shoot = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

	set_virtual_cursor_position(&gamestate->input.cursor,
								gamestate->screen_scale);
	set_virtual_cursor_position(&gamestate->input.cursor_2,
								gamestate->screen_scale);
}

// return 1 if the controls to exit the game are being pressed, 0 otherwise
int exit_control_pressed()
{
	return IsKeyPressed(KEY_ESCAPE) ||
		   (IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT) &&
			IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
}

static int key_vertical_input(Keystate keys);
static int key_horizontal_input(Keystate keys);

Vector2 total_input(Inputstate input, int player_index)
{
	Vector2 total = {0};
	Vector2 raw = {0};

	if (player_index == 0) {
		// add all input together
		raw = (Vector2){
			.x = key_horizontal_input(input.keys) + input.controller.joystick.x,
			.y = key_vertical_input(input.keys) + input.controller.joystick.y};
	} else if (player_index == 1) {
		// add all input together
		raw = (Vector2){.x = key_horizontal_input(input.keys_2) +
							 input.controller_2.joystick.x,
						.y = key_vertical_input(input.keys_2) +
							 input.controller_2.joystick.y};
	}
#ifndef RELEASE
	else {
		printf("ERROR: Unknown player index %d\n", player_index);
		exit(EXIT_FAILURE);
	}
#endif

	// clamp it and return it
	total.x = Clamp(raw.x, -1, 1);
	total.y = Clamp(raw.y, -1, 1);
	return total;
}

static int key_vertical_input(Keystate keys) { return keys.up - keys.down; }
static int key_horizontal_input(Keystate keys)
{
	return keys.left - keys.right;
}
