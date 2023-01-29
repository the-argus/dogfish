#include "input.h"
#include "raylib.h"
#include "raymath.h"
#include "constants.h"

void set_virtual_mouse_position(struct Gamestate *gamestate,
								float screen_scale_fraction)
{
	// get the mouse position as if the window was not scaled from render size
	gamestate->input.mouse.virtual_position.x =
		(gamestate->input.mouse.position.x -
		 (GetScreenWidth() - (GAME_WIDTH * screen_scale_fraction)) * 0.5f) /
		screen_scale_fraction;
	gamestate->input.mouse.virtual_position.y =
		(gamestate->input.mouse.position.y -
		 (GetScreenHeight() - (GAME_HEIGHT * screen_scale_fraction)) * 0.5f) /
		screen_scale_fraction;

	// clamp the virtual mouse position to the size of the rendertarget
	gamestate->input.mouse.virtual_position =
		Vector2Clamp(gamestate->input.mouse.virtual_position, (Vector2){0, 0},
					 (Vector2){(float)GAME_WIDTH, (float)GAME_HEIGHT});
}
