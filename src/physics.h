#pragma once
#include "ode/ode.h"
#include "raylib.h"

void init_physics();
void update_physics(float delta_time);
void close_physics();

const dReal *get_test_cube_position();
const Vector3 get_test_cube_size();
