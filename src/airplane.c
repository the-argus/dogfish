#include "airplane.h"
#include "physics.h"
#include "gameobject.h"

// Like a constructor:
// Hook up all the GameObject optional features
// Attach the chosen inputs to the update method
// player = 0 for p1, 1 for p2
void setup_airplane(GameObject *plane, Gamestate *gamestate,
					unsigned int player)
{
    // plane = create_game_object();
	// Set up the values for the opts
    // DRAW
	// plane->draw.value = &airplane_draw;
    // UPDATE
	if (player == 0) {
		plane->update.value = &airplane_update_p1;
	} else {
		plane->update.value = &airplane_update_p2;
	}

	// Say that it has them
	plane->draw.has = 0;
	plane->update.has = 1;
}

// Accept input state and move accordingly
void airplane_update_p1(struct GameObject *self, Gamestate *gamestate,
						float delta_time)
{
    UNUSED(delta_time);
    apply_airplane_input_impulses(self->physics.value.body.value, gamestate->input.keys, gamestate->input.controller);
}

void airplane_update_p2(struct GameObject *self, Gamestate *gamestate,
						float delta_time)
{
    UNUSED(delta_time);
    apply_airplane_input_impulses(self->physics.value.body.value, gamestate->input.keys_2, gamestate->input.controller_2);
}

// Draw the plane
// void airplane_draw(struct GameObject *self, Gamestate *gamestate) {}
