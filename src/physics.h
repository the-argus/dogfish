#pragma once
#include "ode/ode.h"
#include "raylib.h"

Vector3 to_raylib(const dVector3 v3);

void init_physics();
void update_physics(float delta_time);
void close_physics();

const dReal *get_test_cube_position();
const Vector3 get_test_cube_size();
