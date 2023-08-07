#pragma once
///
/// Includes getters for global variables. Some """thread-safety""" is present,
/// but it's really half-assed and only intended for debugging, to remind us
/// that only on thread should ever be a writer to each of these variables.
///
/// Keep  this file small- it's included in a lot of places.
///

#include "architecture.h"
#include "threadutils.h"
#include <stdatomic.h>
#include <stdbool.h>

/// Do not touch these!!!
extern Inputstate private_inputs;
extern float private_screen_scale;
extern ObjectStructure* private_objects;
extern FullCamera private_p1_camera;
extern FullCamera private_p2_camera;
extern atomic_bool private_inputs_owned;
extern atomic_bool private_objects_owned;
extern atomic_bool private_p1_camera_owned;
extern atomic_bool private_p2_camera_owned;

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

/// Get objects read-only
inline const ObjectStructure* gamestate_get_objects()
{
	return private_objects;
}

/// Get mutable ownership of the objects
inline ObjectStructure* gamestate_get_objects_mutable()
{
	if (private_objects_owned) {
		threadutils_exit(EXIT_FAILURE);
	}
	private_objects_owned = true;
	return private_objects;
}

/// Return ownership of the inputstate
inline void gamestate_return_objects_mutable()
{
	private_objects_owned = false;
}

/// Get player one camera read-only
inline const FullCamera* gamestate_get_p1_camera()
{
	return &private_p1_camera;
}
/// Get mutable ownership of player one camera
inline FullCamera* gamestate_get_p1_camera_mutable()
{
	if (private_inputs_owned) {
		threadutils_exit(EXIT_FAILURE);
	}
	private_p1_camera_owned = true;
	return &private_p1_camera;
}
/// Return ownership of the player one camera. Stop using it after this!
inline void gamestate_return_p1_camera_mutable()
{
	private_p1_camera_owned = false;
}

/// Get player two camera read-only
inline const FullCamera* gamestate_get_p2_camera()
{
	return &private_p2_camera;
}
/// Get mutable ownership of player two camera
inline FullCamera* gamestate_get_p2_camera_mutable()
{
	if (private_inputs_owned) {
		threadutils_exit(EXIT_FAILURE);
	}
	private_p2_camera_owned = true;
	return &private_p2_camera;
}
/// Return ownership of the player two camera. Stop using it after this!
inline void gamestate_return_p2_camera_mutable()
{
	private_p2_camera_owned = false;
}
