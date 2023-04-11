#include "airplane.h"

// Like a constructor:
// Hook up all the GameObject optional features
// Attach the chosen inputs to the update method
void setup_airplane(GameObject *plane, Gamestate *gamestate,
					ControllerState *controller, Cursorstate *cursor,
					Keystate *keys)
{
    // Set up the values for the opts
    plane->draw.value = &airplane_draw;
    plane->update.value = &airplane_update;

    // Say that it has them
    plane->draw.has = 1;
    plane->update.has = 1;
}

// Accept input state and move accordingly
void airplane_update(struct GameObject *self, Gamestate *gamestate, float delta_time)
{
}

void airplane_update_p2(struct GameObject *self, Gamestate *gamestate, float delta_time)
{
    gamestate->input.controller_2;
}

// Draw the plane
void airplane_draw(struct GameObject *self, Gamestate *gamestate) {}