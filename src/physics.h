#pragma once
#include "ode/ode.h"
#include "architecture.h"
#include "raylib.h"

Vector3 to_raylib(const dVector3 v3);

void init_physics(Gamestate *gamestate);
void update_physics(float delta_time);
void close_physics();
int on_ground(dBodyID body);
