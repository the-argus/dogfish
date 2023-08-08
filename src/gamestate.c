#include "gamestate.h"
#include "camera.h"
#include <assert.h>

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
