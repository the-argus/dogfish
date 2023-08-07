#include "bullet.h"
#include "airplane.h"
#include "architecture.h"
#include "constants.h"
#include "gameobject.h"
#include "input.h"
#include "object_structure.h"
#include "physics.h"
#include "render_pipeline.h"
#include "gamestate.h"
#include "skybox.h"
#include "terrain.h"
#include "threadutils.h"

#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>

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

	threadutils_init();

	window_settings();

	gamestate_init();

	// set the update function to run once without doing anything
	update_function = &defer_update_once;
	// set window vars like title and size

	render_pipeline_init();

	physics_init();

	// load skybox textures
	skybox_load();

	terrain_load();

	// initialization complete -------------------------------------------------

	TraceLog(LOG_INFO, "dogfish...");

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

		render_pipeline_gather_screen_info();
		// set the variables in gamestate to reflect input state
		input_gather();

		// update in-game elements before drawing
		update_function();

		// render both cameras to the window
		render(main_draw);
	}

	// cleanup
	object_structure_destroy(&objects);
	terrain_cleanup();
	render_pipeline_cleanup();
	physics_close();
	CloseWindow();

	return 0;
}

/// Perform per-frame game logic.
void update()
{
	// fps_camera_update(gamestate.p1_camera, &(gamestate.p1_camera_data),
	// 				  gamestate.input.cursor);
	// fps_camera_update(gamestate.p2_camera, &(gamestate.p2_camera_data),
	// 				  gamestate.input.cursor_2);

	physics_update(GetFrameTime());

	for (int i = 0; i < object_structure_size(&objects); i++) {
		if (objects._dynarray.head[i].update.has) {
			objects._dynarray.head[i].update.value(&objects._dynarray.head[i],
												   &gamestate, GetFrameTime());
		}
	}

	object_structure_flush_create_queue(gamestate.objects);
}

/// Draw the in-game objects to a consistently sized rendertexture.
void main_draw()
{
	skybox_draw();

	terrain_draw();

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
	SetConfigFlags((uint32_t)FLAG_WINDOW_RESIZABLE | (uint32_t)FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "dogfish");
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	// Lock cursor for first person and third person cameras
	DisableCursor();
}
