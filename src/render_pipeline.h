#pragma once

#include "architecture.h"

void gather_screen_info(Gamestate *gamestate);
void init_render_pipeline();
void render(Gamestate gamestate, void (*game_draw)());
void cleanup_render_pipeline();
