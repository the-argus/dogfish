#pragma once
#include "architecture.h"

// contain all this in a function because we don't want gamestate to be
// modified during this process
void set_virtual_mouse_position(struct Gamestate *gamestate,
								float screen_scale_fraction);

void gather_input(Gamestate* gamestate, float screen_scaling);

int exit_control_pressed();

Vector2 total_input(Inputstate input, int player_index);
