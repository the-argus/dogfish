#pragma once

#include "gameobject.h"

GameObject create_bullet(Gamestate *gamestate);
void bullet_update(GameObject* self, Gamestate *gamestate, float delta_time);
void bullet_draw(GameObject* self, Gamestate *gamestate);
