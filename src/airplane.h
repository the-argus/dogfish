#pragma once
#include "raylib.h"
#include "architecture.h"

// Like a constructor:
// Hook up all the GameObject optional features
// Attach the chosen inputs to the update method
void setup_airplane(GameObject *plane, Gamestate *gamestate, ControllerState *controller,
					Cursorstate *cursor, Keystate *keys, unsigned int player);

// Accept p1's input state from game state, and move accordingly
void airplane_update_p1(struct GameObject *self, Gamestate *gamestate, float delta_time);

// Accept p2's input state from game state, and move accordingly
void airplane_update_p2(struct GameObject *self, Gamestate *gamestate, float delta_time);

// Draw the plane
void airplane_draw();