#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "constants.h"
#include "architecture.h"
#include "input.h"

// useful for screen scaling
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int main(void)
{
	bool window_open = true;

	SetTargetFPS(60);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "lungfish");
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	// variable width screen
	float scale;
	RenderTexture2D main_target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
	SetTextureFilter(main_target.texture, TEXTURE_FILTER_BILINEAR);

	// inialize gamestate
	struct Gamestate *gamestate;
	gamestate = malloc(sizeof(struct Gamestate));
	if (gamestate == NULL) {
		printf("Malloc failed\n");
		return 1;
	}

	// init some of the gamestate values
	gamestate->input.mouse.virtual_position = (Vector2){0};

	// initialization complete
	printf("lungfish...\n");

	while (window_open) {
		// find what window resizing is necessary
		scale = MIN((float)GetScreenWidth() / GAME_WIDTH,
					(float)GetScreenHeight() / GAME_HEIGHT);

		// collect mouse information
		gamestate->input.mouse.position = GetMousePosition();
		set_virtual_mouse_position(gamestate, scale);

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			gamestate->input.mouse.left_pressed = 1;
		} else {
			gamestate->input.mouse.left_pressed = 0;
		}
		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			gamestate->input.mouse.right_pressed = 1;
		} else {
			gamestate->input.mouse.right_pressed = 0;
		}

		if (IsKeyPressed(KEY_ESCAPE) || WindowShouldClose()) {
			window_open = false;
		}

		// draw to render texture
		BeginTextureMode(main_target);
		ClearBackground(RAYWHITE);
		EndTextureMode();

		BeginDrawing();
		// color of the bars around the rendertexture
		ClearBackground(BLACK);
		// draw the render texture scaled
		DrawTexturePro(
			main_target.texture,
			(Rectangle){0.0f, 0.0f, (float)main_target.texture.width,
						(float)-main_target.texture.height},
			(Rectangle){(GetScreenWidth() - ((float)GAME_WIDTH * scale)) * 0.5f,
						(GetScreenHeight() - ((float)GAME_HEIGHT * scale)) *
							0.5f,
						(float)GAME_WIDTH * scale, (float)GAME_HEIGHT * scale},
			(Vector2){0, 0}, 0.0f, WHITE);
		EndDrawing();
	}

	// cleanup
	free(gamestate);
	CloseWindow();

	return 0;
}
