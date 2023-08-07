#include "gameobject.h"

static int unused_id = 1;

PhysicsComponent create_physics_component()
{
	return (PhysicsComponent){.bit = 0,
							  .body = {.has = 0, .value = NULL},
							  .geom = NULL,
							  .mask = 0,
							  .collision_handler = {.has = 0, .value = NULL}};
}

GameObject create_game_object()
{
	unused_id += 1;
	return (GameObject){
		.queued_for_cleanup = 0,
		.id = unused_id - 1,
		.physics = {.has = 0, .value = create_physics_component()},
		.draw = {.has = 0, .value = NULL},
		.cleanup = {.has = 0, .value = NULL},
		.update = {.has = 0, .value = NULL},
		.disabled = 0,
	};
}
