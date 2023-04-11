#include "airplane.h"

// Like a constructor:
// Hook up all the GameObject optional features
// Attach the chosen inputs to the update method
// player = 0 for p1, 1 for p2
void setup_airplane(GameObject *plane, Gamestate *gamestate,
					ControllerState *controller, Cursorstate *cursor,
					Keystate *keys, unsigned int player)
{
	// Set up the values for the opts
	plane->draw.value = &airplane_draw;
	if (player == 0) {
		plane->update.value = &airplane_update_p1;
	}
    else {
        plane->update.value = &airplane_update_p2;
    }

	// Say that it has them
	plane->draw.has = 1;
	plane->update.has = 1;
}

// Accept input state and move accordingly
void airplane_update_p1(struct GameObject *self, Gamestate *gamestate,
						float delta_time)
{
}

void airplane_update_p2(struct GameObject *self, Gamestate *gamestate,
						float delta_time)
{
	gamestate->input.controller_2;
}

// Draw the plane
void airplane_draw(struct GameObject *self, Gamestate *gamestate) {}