#include "input.h"
#include "constants.h"
#include "gamestate.h"
#include <assert.h>
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
	input->keys[0].right = IsKeyDown(KEY_D);
	input->keys[0].left = IsKeyDown(KEY_A);
	input->keys[0].up = IsKeyDown(KEY_S);
	input->keys[0].down = IsKeyDown(KEY_W);
	input->keys[0].boost = IsKeyDown(KEY_SPACE);

	// player 2 moves with arrow keys
	input->keys[1].right = IsKeyDown(KEY_RIGHT);
	input->keys[1].left = IsKeyDown(KEY_LEFT);
	input->keys[1].up = IsKeyDown(KEY_DOWN);
	input->keys[1].down = IsKeyDown(KEY_UP);
	input->keys[1].boost = IsKeyDown(KEY_ENTER);

	// collect controller information

	// player 1 uses d-pad and left joystick
	input->controller[0].boost =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP);
	input->controller[0].shoot =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

	input->controller[0].joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
	input->controller[0].joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

	// player 2 uses buttons (X Y A B) and right joystick
	input->controller[1].boost =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
	input->controller[1].shoot =
		IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

	input->controller[1].joystick.x =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
	input->controller[1].joystick.y =
		GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

	// collect cursor information
	// TODO: maybe remove the second cursorstate object if we continue to have
	// the mouse only work for player 1
	input->cursor[0].delta = GetMouseDelta();
	input->cursor[0].position = GetMousePosition();
	input->cursor[0].shoot = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
	set_virtual_cursor_position(&input->cursor[0]);

	gamestate_return_inputstate_mutable();
}

// return 1 if the controls to exit the game are being pressed, 0 otherwise
int exit_control_pressed()
{
	return IsKeyPressed(KEY_ESCAPE) ||
		   (IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT) &&
			IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
}

static inline int key_vertical_input(const Keystate* keys)
{
	return keys->up - keys->down;
}
static inline int key_horizontal_input(const Keystate* keys)
{
	return keys->left - keys->right;
}

Vector2 total_input(uint8_t player_index)
{
	assert(player_index >= 0 && player_index <= 1);
	const Inputstate* input = gamestate_get_inputstate();
	Vector2 total = {0};
	Vector2 raw = (Vector2){
		.x = (float)key_horizontal_input(&input->keys[player_index]) +
			 input->controller[player_index].joystick.x,
		.y = (float)key_vertical_input(&input->keys[player_index]) +
			 input->controller[player_index].joystick.y,
	};

	total.x = Clamp(raw.x, -1, 1);
	total.y = Clamp(raw.y, -1, 1);
	return total;
}

Vector2 total_cursor(uint8_t player_index)
{
	const Inputstate* input = gamestate_get_inputstate();
	return (Vector2){
		input->cursor[player_index].delta.x +
			input->controller[player_index].joystick.x,
		input->cursor[player_index].delta.y +
			input->controller[player_index].joystick.y,
	};
}
