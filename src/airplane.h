#pragma once
#include "raylib.h"
#include "architecture.h"

// Like a constructor:
// Hook up all the GameObject optional features
// Attach the chosen inputs to the update method
void setup_airplane(GameObject *plane, 
					ControllerState *controller, Cursorstate *cursor,
					Keystate *keys);

// Accept input state and move accordingly
void airplane_update(ControllerState *controller, Cursorstate *cursor,
					 Keystate *keys);

// Draw the plane
void airplane_draw();