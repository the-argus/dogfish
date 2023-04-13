#include <stdio.h>
#include <stdlib.h>

#include "bullet.h"
#include "raylib.h"
#include "raymath.h"

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

// useful for screen scaling
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// the big important things
static Gamestate gamestate;		// stores miscellaneous variables
static ObjectStructure objects; // contains all the game objects

// window scaling and splitscreen
static RenderTexture2D main_target;
static RenderTexture2D rt1;
static RenderTexture2D rt2;
// make sure to take absolute values when using height...
static const Rectangle splitScreenRect = {
	.x = 0, .y = 0, .width = GAME_WIDTH, .height = (int)-(GAME_HEIGHT / 2)};

// use a function pointer for the update loop so that we can change it
static void (*update_function)();

// split up code that would normally all be in main()
void window_settings();
void init_rendertextures();
void update();
void main_draw();
void window_draw();
void defer_update_once() { update_function = &update; }

int main(void)
{
	// initialize miscellaneous things from other functions and files ----------

	// set the update function to run once without doing anything
	update_function = &defer_update_once;
	// set windowing backend vars like title and size
	window_settings();
	// initialize main_texture render texture to the correct size
	init_rendertextures();
	// load skybox textures
	load_skybox();
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

		// fraction of window resize that will occur this frame, basically the
		// difference between current width/height ratio to target width/height
		gamestate.screen_scale = MIN((float)GetScreenWidth() / GAME_WIDTH,
									 (float)GetScreenHeight() / GAME_HEIGHT);

		// set the variables in gamestate to reflect input state
		gather_input(&gamestate);

		// update in-game elements before drawing
		update_function();

		// Render Camera 1
		BeginTextureMode(rt1);
		// clang-format off
		    ClearBackground(RAYWHITE);
            BeginMode3D(*gamestate.p1_camera);
                // draw in-game objects
                main_draw();

            EndMode3D();
		// clang-format on
		EndTextureMode();

		// Render Camera 2
		BeginTextureMode(rt2);
		// clang-format off
		    ClearBackground(RAYWHITE);
            BeginMode3D(*gamestate.p2_camera);
                // draw in-game objects
                main_draw();

            EndMode3D();
		// clang-format on
		EndTextureMode();

		// set draw target to the rendertexture, dont actually draw to window
		BeginTextureMode(main_target);
		ClearBackground(BLACK);
		DrawTextureRec(rt1.texture, splitScreenRect, (Vector2){0, 0}, WHITE);
		DrawTextureRec(rt2.texture, splitScreenRect,
					   (Vector2){0, abs((int)splitScreenRect.height)}, WHITE);
		EndTextureMode();

		// draw the game to the window at the correct size
		BeginDrawing();
		window_draw();
		EndDrawing();
	}

	// cleanup
	while (object_structure_size(&objects) > 0) {
		object_structure_remove(&objects, 0);
	}
	free(gamestate.p1_camera);
	free(gamestate.p2_camera);
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

	for (int i = 0; i < object_structure_size(&objects); i++) {
		if (objects._dynarray.head[i].draw.has) {
			objects._dynarray.head[i].draw.value(&objects._dynarray.head[i],
												 &gamestate);
		}
	}

	const int size = 50;
	const float unit = 1;
	const float threshhold = 0.5f;
	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			for (int z = 0; z < size; z++) {
				if (perlin(x, y, z, 2, 10, 3) > threshhold) {
					DrawCube((Vector3){x, y, z}, unit, unit, unit, GREEN);
				}
			}
		}
	}

	// grid for visual aid
	DrawGrid(10, 1.0f);
}

/// Resize the game's main render texture and draw it to the window.
void window_draw()
{
	// color of the bars around the rendertexture
	ClearBackground(BLACK);
	// draw the render texture scaled
	DrawTexturePro(main_target.texture,
				   (Rectangle){0.0f, 0.0f, (float)main_target.texture.width,
							   (float)-main_target.texture.height},
				   (Rectangle){(GetScreenWidth() -
								((float)GAME_WIDTH * gamestate.screen_scale)) *
								   0.5f,
							   (GetScreenHeight() -
								((float)GAME_HEIGHT * gamestate.screen_scale)) *
								   0.5f,
							   (float)GAME_WIDTH * gamestate.screen_scale,
							   (float)GAME_HEIGHT * gamestate.screen_scale},
				   (Vector2){0, 0}, 0.0f, WHITE);
}

/// Set windowing backend settings like window title and size.
void window_settings()
{
	SetTargetFPS(60);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "dogfish");
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
}

/// Initialize the main rendertexture to which the actual game elements are
/// drawn. Determines the universal texture filter for scaling.
void init_rendertextures()
{
	// variable width screen
	main_target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);

	rt1 = LoadRenderTexture(abs((int)splitScreenRect.width),
							abs((int)splitScreenRect.height));
	rt2 = LoadRenderTexture(abs((int)splitScreenRect.width),
							abs((int)splitScreenRect.height));

	// set all to bilinear
	SetTextureFilter(main_target.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(rt1.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(rt2.texture, TEXTURE_FILTER_BILINEAR);
}
