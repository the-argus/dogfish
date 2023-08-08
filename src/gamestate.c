#include "gamestate.h"
#include "camera.h"
#include "threadutils.h"
#include <assert.h>
#include <stdatomic.h>

atomic_bool initialized = false;

Inputstate private_inputs;
float private_screen_scale;
FullCamera private_cameras[2];
atomic_bool private_inputs_owned;
atomic_bool private_cameras_owned;

void gamestate_init()
{
	assert(!initialized);
	initialized = true;

	for (uint8_t i = 0; i < 2; ++i) {
		camera_new(&private_cameras[i]);
	}

	private_inputs_owned = false;
	private_cameras_owned = false;

	private_screen_scale = 0;

	private_inputs = (Inputstate){
		.cursor =
			{
				(Cursorstate){
					.delta = 0,
					.shoot = false,
					.position = (Vector2){0, 0},
					.virtual_position = (Vector2){0, 0},
				},
				(Cursorstate){
					.delta = 0,
					.shoot = false,
					.position = (Vector2){0, 0},
					.virtual_position = (Vector2){0, 0},
				},
			},
		.keys =
			{
				(Keystate){
					.up = false,
					.down = false,
					.left = false,
					.right = false,
				},
				(Keystate){
					.up = false,
					.down = false,
					.left = false,
					.right = false,
				},
			},
		.controller =
			{
				(ControllerState){
					.shoot = false,
					.boost = false,
					.joystick = (Joystick){0, 0},
				},
				(ControllerState){
					.shoot = false,
					.boost = false,
					.joystick = (Joystick){0, 0},
				},
			},
	};
}

/// Get inputstate read-only
const Inputstate* gamestate_get_inputstate() { return &private_inputs; }
/// Get mutable ownership of inputstate
Inputstate* gamestate_get_inputstate_mutable()
{
	if (private_inputs_owned) {
		threadutils_exit(EXIT_FAILURE);
	}
	private_inputs_owned = true;
	return &private_inputs;
}
/// Return ownership of the inputstate. Stop using it after this!
void gamestate_return_inputstate_mutable() { private_inputs_owned = false; }

/// Get screen scale copy
float gamestate_get_screen_scale() { return private_screen_scale; }
/// Set the screen scale
void gamestate_set_screen_scale(float new_scale)
{
	private_screen_scale = new_scale;
}

/// Get player one camera read-only
const FullCamera* gamestate_get_cameras() { return private_cameras; }
/// Get mutable ownership of player one camera
FullCamera* gamestate_get_cameras_mutable()
{
	if (private_cameras_owned) {
		threadutils_exit(EXIT_FAILURE);
	}
	private_cameras_owned = true;
	return private_cameras;
}
/// Return ownership of the player one camera. Stop using it after this!
void gamestate_return_cameras_mutable() { private_cameras_owned = false; }
