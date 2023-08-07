#pragma once
#include <raylib.h>
#include <stdint.h>

void input_gather();

int exit_control_pressed();

Vector2 total_input(uint8_t player_index);
