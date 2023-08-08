#pragma once
///
/// Includes getters for global variables. Some """thread-safety""" is present,
/// but it's really half-assed and only intended for debugging, to remind us
/// that only on thread should ever be a writer to each of these variables.
///

#include "camera.h"
#include "input.h"
#include <stdlib.h>

/// Initialize global variables
void gamestate_init();

/// Get inputstate read-only
const Inputstate* gamestate_get_inputstate();
/// Get mutable ownership of inputstate
Inputstate* gamestate_get_inputstate_mutable();
/// Return ownership of the inputstate. Stop using it after this!
void gamestate_return_inputstate_mutable();
/// Get screen scale copy
float gamestate_get_screen_scale();
/// Set the screen scale
void gamestate_set_screen_scale(float new_scale);
/// Get player one camera read-only
const FullCamera* gamestate_get_cameras();
/// Get mutable ownership of player one camera
FullCamera* gamestate_get_cameras_mutable();
/// Return ownership of the player one camera. Stop using it after this!
void gamestate_return_cameras_mutable();
