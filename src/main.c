#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "constants.h"
#include "architecture.h"

int main(void)
{
	bool window_open = true;
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "lungfish");

	SetTargetFPS(60);

	// inialize gamestate
	struct Gamestate *gamestate;
	gamestate = malloc(sizeof(struct Gamestate));
	if (gamestate == NULL) {
		printf("Malloc failed\n");
		return 1;
	}

	// initialization complete
	printf("lungfish...\n");

	while (window_open) {

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

		BeginDrawing();

		ClearBackground(RAYWHITE);

		EndDrawing();
	}

	// cleanup
	free(gamestate);
	CloseWindow();

	return 0;
}
