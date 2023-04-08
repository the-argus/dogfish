#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#include "constants.h"
#include "architecture.h"
#include "input.h"
#include "camera_manager.h"
#include "fps_camera.h"
#include "physics.h"
#include "skybox.h"
#include "debug.h"

// useful for screen scaling
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static Gamestate gamestate;
static float scale;
static RenderTexture2D main_target;
static void (*update_function)();

void window_settings();
void init_rendertexture();
void gather_input();
void update();
void main_draw();
void window_draw();
void defer_update_once() { update_function = &update; }

int main(void)
{
    update_function = &defer_update_once;
	// set windowing backend vars like title and size
	window_settings();
	// initialize main_texture to the correct size
	init_rendertexture();

	load_skybox();

	// initialize physics system
	init_physics();

	// inialize gamestate struct
	gamestate.input.mouse.virtual_position = (Vector2){0};
	// this initializes current_camera, which involves a malloc
	gamestate_new_fps_camera(&gamestate);

	// initialization complete
	printf("dogfish...\n");

	// loop until player presses escape or close button
	bool window_open = true;
	while (window_open) {
		if (IsKeyPressed(KEY_ESCAPE) || WindowShouldClose()) {
			window_open = false;
		}

		// fraction of window resize that will occur this frame, basically the
		// difference between current width/height ratio to target width/height
		scale = MIN((float)GetScreenWidth() / GAME_WIDTH,
					(float)GetScreenHeight() / GAME_HEIGHT);

		// set the variables in gamestate to reflect input state
		gather_input();

		// update in-game elements before drawing
		update_function();

		// set draw target to the rendertexture, dont actually draw to window
		BeginTextureMode(main_target);
		// clang-format off
		    ClearBackground(RAYWHITE);
            BeginMode3D(*gamestate.current_camera);
                // draw in-game objects
                main_draw();

            EndMode3D();
		// clang-format on
		EndTextureMode();

		// draw the game to the window at the correct size
		BeginDrawing();
		window_draw();
		EndDrawing();
	}

	// cleanup
	free(gamestate.current_camera);
	close_physics();
	CloseWindow();

	return 0;
}

/// Perform per-frame game logic.
void update()
{
	fps_camera_update(gamestate.current_camera, &(gamestate.camera_data));
	update_camera_tilt(gamestate.current_camera, gamestate.input);

    apply_player_input_impulses(gamestate.input, gamestate.camera_data.angle.x);
	update_physics(GetFrameTime());
    
    Vector3 pos = to_raylib(get_test_cube_position());
    gamestate.current_camera->position = pos;
}

/// Draw the in-game objects to a consistently sized rendertexture.
void main_draw()
{
	draw_skybox();
	// draw a cube
    Vector3 pos = to_raylib(get_test_cube_position());
    Vector3 size = get_test_cube_size();
	DrawCube(pos, size.x, size.y, size.z, RED);

	// grid for visual aid
	DrawGrid(10, 1.0f);
}

/// Resize the game's main render texture and draw it to the window.
void window_draw()
{
	// color of the bars around the rendertexture
	ClearBackground(BLACK);
	// draw the render texture scaled
	DrawTexturePro(
		main_target.texture,
		(Rectangle){0.0f, 0.0f, (float)main_target.texture.width,
					(float)-main_target.texture.height},
		(Rectangle){(GetScreenWidth() - ((float)GAME_WIDTH * scale)) * 0.5f,
					(GetScreenHeight() - ((float)GAME_HEIGHT * scale)) * 0.5f,
					(float)GAME_WIDTH * scale, (float)GAME_HEIGHT * scale},
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
void init_rendertexture()
{
	// variable width screen
	main_target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
	SetTextureFilter(main_target.texture, TEXTURE_FILTER_BILINEAR);
}

/// Make the gamestate reflect the actual system IO state.
void gather_input()
{
	// collect mouse information
	gamestate.input.mouse.position = GetMousePosition();
	set_virtual_mouse_position(&gamestate, scale);

	gamestate.input.mouse.left_pressed =
		IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	gamestate.input.mouse.right_pressed =
		IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

	// collect keyboard information
    gamestate.input.keys.jump = IsKeyDown(KEY_SPACE);
	gamestate.input.keys.right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
	gamestate.input.keys.left = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
	gamestate.input.keys.up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
	gamestate.input.keys.down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
}
