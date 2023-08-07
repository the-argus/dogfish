#include "input.h"
#include "architecture.h"
#include "constants.h"
#include "gamestate.h"
#include <raylib.h>
#include <raymath.h>

/// Makes sure the location of the virtual cursor respects whatever the
/// cursor's actual position is
static void set_virtual_cursor_position(Cursorstate* cursor)
{
	// get the mouse position as if the window was not scaled from render size
	cursor->virtual_position.x =
		(cursor->position.x -
		 (GetScreenWidth() - (GAME_WIDTH * gamestate_get_screen_scale())) *
			 0.5f) /
		gamestate_get_screen_scale();
	cursor->virtual_position.y =
		(cursor->position.y -
		 (GetScreenHeight() - (GAME_HEIGHT * gamestate_get_screen_scale())) *
			 0.5f) /
		gamestate_get_screen_scale();

	// clamp the virtual mouse position to the size of the rendertarget
	cursor->virtual_position =
		Vector2Clamp(cursor->virtual_position, (Vector2){0, 0},
					 (Vector2){(float)GAME_WIDTH, (float)GAME_HEIGHT});
}

/// Make the gamestate reflect the actual system IO state.
void input_gather()
{
	Inputstate* input = gamestate_get_inputstate_mutable();

	// collect keyboard information

	// player 1 moves with WASD
	input->keys.right = IsKeyDown(KEY_D);
	input->keys.left = IsKeyDown(KEY_A);
	input->keys.up = IsKeyDown(KEY_W);
	input->keys.down = IsKeyDown(KEY_S);
	input->keys.boost = IsKeyDown(KEY_SPACE);

	// player 2 moves with arrow keys
	input->keys_2.right = IsKeyDown(KEY_RIGHT);
	input->keys_2.left = IsKeyDown(KEY_LEFT);
	input->keys_2.up = IsKeyDown(KEY_UP);
	input->keys_2.down = IsKeyDown(KEY_DOWN);
	input->keys_2.boost = IsKeyDown(KEY_ENTER);

	// collect controller information

	// player 1 uses d-pad and left joystick
	input->controller.boost =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP);
	input->controller.shoot =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

	input->controller.joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
	input->controller.joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

	// player 2 uses buttons (X Y A B) and right joystick
	input->controller_2.boost =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
	input->controller_2.shoot =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

	input->controller_2.joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
	input->controller_2.joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

	// collect cursor information (mouse input only affects player 1's cursor)
	// *delta* is how much the cursor should move this frame. used by the first
	// person cameras to determine how much to rotate.
	input->cursor.delta =
		Vector2Add(GetMouseDelta(), input->controller.joystick);
	input->cursor_2.delta = input->controller_2.joystick;

	// *position* is where the cursor is on the screen. will be useful for
	// things like pause menus.
	input->cursor.position = GetMousePosition();

	input->cursor.shoot = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

	set_virtual_cursor_position(&input->cursor);
	set_virtual_cursor_position(&input->cursor_2);

	gamestate_return_inputstate_mutable();
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

Vector2 total_input(uint8_t player_index)
{
	const Inputstate* input = gamestate_get_inputstate();
	Vector2 total = {0};
	Vector2 raw = {0};

	if (player_index == 0) {
		// add all input together
		raw = (Vector2){.x = key_horizontal_input(input->keys) +
							 input->controller.joystick.x,
						.y = key_vertical_input(input->keys) +
							 input->controller.joystick.y};
	} else if (player_index == 1) {
		// add all input together
		raw = (Vector2){.x = key_horizontal_input(input->keys_2) +
							 input->controller_2.joystick.x,
						.y = key_vertical_input(input->keys_2) +
							 input->controller_2.joystick.y};
	}
#ifndef RELEASE
	else {
		TraceLog(LOG_DEBUG, "ERROR: Unknown player index %d", player_index);
		threadutils_exit(EXIT_FAILURE);
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
