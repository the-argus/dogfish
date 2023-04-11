#pragma once
#include "ode/ode.h"
#include "architecture.h"
#include "raylib.h"

Vector3 to_raylib(const dVector3 v3);

void init_physics(Gamestate *gamestate);
void update_physics(float delta_time);
void close_physics();

const dReal *get_test_cube_position();
Vector3 get_test_cube_size();

void apply_player_input_impulses(Inputstate inputstate, float angle_x);
void apply_airplane_input_impulses(Inputstate inputstate, float angle_x, unsigned int player_index);