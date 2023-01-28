#include <stdio.h>
#include "raylib.h"
#include "constants.h"

int main(void)
{
    char window_open = 1;
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "lungfish");

    SetTargetFPS(60);

	printf("lungfish...\n");

    while (window_open) {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        EndDrawing();
    }

    CloseWindow();

	return 0;
}
