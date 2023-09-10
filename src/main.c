#include "airplane.h"
#include "bullet.h"
#include "constants/screen.h"
#include "gamestate.h"
#include "input.h"
#include "render_pipeline.h"
#include "skybox.h"
#include "terrain.h"
#include "threadutils.h"

// use a function pointer for the update loop so that we can change it
static void (*update_function)();

// split up code that would normally all be in main()
static void window_settings();
static void update();
static void main_draw();
static void defer_update_once() { update_function = &update; }

int main(void)
{
	// intialize opengl context first
	window_settings();
	// initialize global mutexes (mutices?)
	threadutils_init();

	// actual game intialization
	gamestate_init();
	render_pipeline_init();
	bullet_init();
	airplane_init();
	skybox_load();
	terrain_load();

	// set the update function to run once without doing anything
	update_function = defer_update_once;

	// initialization complete

	TraceLog(LOG_INFO, "dogfish...");

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
	bullet_cleanup();
	airplane_cleanup();
	render_pipeline_cleanup();
	skybox_cleanup();
	terrain_cleanup();
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

	bullet_update();
	airplane_update(GetFrameTime());
	terrain_update();
}

/// Draw the in-game objects to a consistently sized rendertexture.
void main_draw()
{
	skybox_draw();
	terrain_draw();
	bullet_draw();
	airplane_draw();

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
