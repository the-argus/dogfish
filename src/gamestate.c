#include "gamestate.h"
#include "camera.h"

atomic_bool initialized = false;

Inputstate private_input;
float private_screen_scale;
ObjectStructure* private_objects;

FullCamera private_p1_camera;
FullCamera private_p2_camera;

atomic_bool private_input_owned;
atomic_bool private_objects_owned;
atomic_bool private_p1_camera_owned;
atomic_bool private_p2_camera_owned;

void gamestate_init()
{
	assert(!initialized);
	initialized = true;

	camera_new(&private_p1_camera);
	camera_new(&private_p2_camera);

	private_input_owned = false;
	private_objects_owned = false;
	private_p1_camera_owned = false;
	private_p2_camera_owned = false;

	private_screen_scale = 0;

	private_objects = NULL;

	private_input = (Inputstate){
		.keys =
			(Keystate){
				.up = false,
				.down = false,
				.left = false,
				.right = false,
			},
		.cursor =
			(Cursorstate){
				.delta = 0,
				.shoot = false,
				.position = (Vector2){0, 0},
				.virtual_position = (Vector2){0, 0},
			},
		.keys_2 =
			(Keystate){
				.up = false,
				.down = false,
				.left = false,
				.right = false,
			},
		.cursor_2 =
			(Cursorstate){
				.delta = 0,
				.shoot = false,
				.position = (Vector2){0, 0},
				.virtual_position = (Vector2){0, 0},
			},
		.controller =
			(ControllerState){
				.shoot = false,
				.boost = false,
				.joystick = (Joystick){0, 0},
			},
		.controller_2 =
			(ControllerState){
				.shoot = false,
				.boost = false,
				.joystick = (Joystick){0, 0},
			},
	};
}
