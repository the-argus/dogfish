#pragma once
#include "architecture.h"

// contain all this in a function because we don't want gamestate to be
// modified during this process
void set_virtual_mouse_position(struct Gamestate *gamestate,
								float screen_scale_fraction);

float get_joystick(ControllerState *cstate);

void gather_input(Gamestate* gamestate, float screen_scaling);
