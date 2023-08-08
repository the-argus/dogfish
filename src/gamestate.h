#pragma once
///
/// Includes getters for global variables. Some """thread-safety""" is present,
/// but it's really half-assed and only intended for debugging, to remind us
/// that only on thread should ever be a writer to each of these variables.
///
/// Keep  this file small- it's included in a lot of places.
///

#include "threadutils.h"
#include "camera.h"
#include "input.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

/// Do not touch these!!!
extern Inputstate private_inputs;
extern float private_screen_scale;
extern FullCamera private_cameras[2];
extern atomic_bool private_inputs_owned;
extern atomic_bool private_cameras_owned;

/// Initialize global variables
void gamestate_init();

/// Get inputstate read-only
inline const Inputstate* gamestate_get_inputstate() { return &private_inputs; }
/// Get mutable ownership of inputstate
inline Inputstate* gamestate_get_inputstate_mutable()
{
	if (private_inputs_owned) {
		threadutils_exit(EXIT_FAILURE);
	}
	private_inputs_owned = true;
	return &private_inputs;
}
/// Return ownership of the inputstate. Stop using it after this!
inline void gamestate_return_inputstate_mutable()
{
	private_inputs_owned = false;
}

/// Get screen scale copy
inline float gamestate_get_screen_scale() { return private_screen_scale; }
/// Set the screen scale
inline void gamestate_set_screen_scale(float new_scale)
{
	private_screen_scale = new_scale;
}

/// Get player one camera read-only
inline const FullCamera* gamestate_get_cameras() { return private_cameras; }
/// Get mutable ownership of player one camera
inline FullCamera* gamestate_get_cameras_mutable()
{
	if (private_cameras_owned) {
		threadutils_exit(EXIT_FAILURE);
	}
	private_cameras_owned = true;
	return private_cameras;
}
/// Return ownership of the player one camera. Stop using it after this!
inline void gamestate_return_cameras_mutable()
{
	private_cameras_owned = false;
}
