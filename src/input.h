#pragma once
#include "architecture.h"

void gather_input(Gamestate* gamestate);

int exit_control_pressed();

Vector2 total_input(Inputstate input, int player_index);
