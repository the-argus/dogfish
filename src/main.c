#include <stdio.h>
#include <stdlib.h>

#include "bullet.h"
#include "raylib.h"
#include "raymath.h"
#include "render_pipeline.h"
#include "rlgl.h"

#include "constants.h"
#include "architecture.h"
#include "input.h"
#include "camera_manager.h"
#include "fps_camera.h"
#include "physics.h"
#include "skybox.h"
#include "airplane.h"
#include "gameobject.h"
#include "object_structure.h"
#include "terrain.h"
#include "debug.h"

// the big important things
static Gamestate gamestate;		// stores miscellaneous variables
static ObjectStructure objects; // contains all the game objects

// use a function pointer for the update loop so that we can change it
static void (*update_function)();

// split up code that would normally all be in main()
static void window_settings();
static void update();
static void main_draw();
static void defer_update_once() { update_function = &update; }

int main(void)
{
	// initialize miscellaneous things from other functions and files ----------

	// set the update function to run once without doing anything
	update_function = &defer_update_once;
	// set windowing backend vars like title and size
	window_settings();

    init_render_pipeline();

	// load skybox textures
	load_skybox();

	load_terrain();
	// allocate memory for the object structure which will contain all
	// gameobjects
	objects = object_structure_create();

	// inialize gamestate struct -----------------------------------------------
	gamestate.input.cursor.virtual_position = (Vector2){0};
	gamestate.input.cursor_2.virtual_position = (Vector2){0};
	// these initialize current_camera, which involves a malloc
	// player 1
	gamestate_new_fps_camera(&gamestate, 0);
	// player 2
	gamestate_new_fps_camera(&gamestate, 1);

	// initialize physics system
	init_physics(&gamestate);

	// initialization complete -------------------------------------------------

	printf("dogfish...\n");

	// create game objects
	GameObject p1_plane = create_airplane(gamestate, 0);
	GameObject p2_plane = create_airplane(gamestate, 1);
	GameObject my_bullet = create_bullet(gamestate);

	// copy the game objects into the object structure (the originals dont
	// matter)
	object_structure_insert(&objects, p1_plane);
	object_structure_insert(&objects, p2_plane);
	object_structure_insert(&objects, my_bullet);

	// loop until player presses escape or close button, or both start/select
	// buttons on a controller
	bool window_open = true;
	while (window_open) {
		if (exit_control_pressed() || WindowShouldClose()) {
			window_open = false;
		}

		gather_screen_info(&gamestate);
		// set the variables in gamestate to reflect input state
		gather_input(&gamestate);

		// update in-game elements before drawing
		update_function();

		// render both cameras to the window
		render(gamestate, main_draw);
	}

	// cleanup
	while (object_structure_size(&objects) > 0) {
		object_structure_remove(&objects, 0);
	}
	free(objects._dynarray.head);
	free(gamestate.p1_camera);
	free(gamestate.p2_camera);
	cleanup_terrain();
	cleanup_render_pipeline();
	close_physics();
	CloseWindow();

	return 0;
}

/// Perform per-frame game logic.
void update()
{
	fps_camera_update(gamestate.p1_camera, &(gamestate.p1_camera_data),
					  gamestate.input.cursor);
	fps_camera_update(gamestate.p2_camera, &(gamestate.p2_camera_data),
					  gamestate.input.cursor_2);

	update_physics(GetFrameTime());

	for (int i = 0; i < object_structure_size(&objects); i++) {
		if (objects._dynarray.head[i].update.has) {
			objects._dynarray.head[i].update.value(&objects._dynarray.head[i],
												   &gamestate, GetFrameTime());
		}
	}
}

/// Draw the in-game objects to a consistently sized rendertexture.
void main_draw()
{
	draw_skybox();

	draw_terrain();

	for (int i = 0; i < object_structure_size(&objects); i++) {
		if (objects._dynarray.head[i].draw.has) {
			objects._dynarray.head[i].draw.value(&objects._dynarray.head[i],
												 &gamestate);
		}
	}

	// grid for visual aid
	DrawGrid(10, 1.0f);
}

/// Set windowing backend settings like window title and size.
static void window_settings()
{
	SetTargetFPS(60);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "dogfish");
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
}
