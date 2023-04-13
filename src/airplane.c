#include "airplane.h"
#include "include/constants.h"
#include "physics.h"
#include "shorthand.h"
#include "gameobject.h"

#define AIRPLANE_DEBUG_CUBE_WIDTH 0.5
#define AIRPLANE_DEBUG_CUBE_LENGTH 2
#define AIRPLANE_DEBUG_CUBE_COLOR RED
#define AIRPLANE_MASS 5.4
#define INITIAL_AIRPLANE_POS_P1 5, 10, 0
#define INITIAL_AIRPLANE_POS_P2 -5, 10, 0

// Accept p1's input state from game state, and move accordingly
static void airplane_update_p1(struct GameObject *self, Gamestate *gamestate,
							   float delta_time);

// Accept p2's input state from game state, and move accordingly
static void airplane_update_p2(struct GameObject *self, Gamestate *gamestate,
							   float delta_time);

// general function that applies airplane-like forces based on keys and controls
static void apply_airplane_input_impulses(dBodyID plane, Keystate keys,
										  ControllerState controls);

// Draw the plane
static void airplane_draw();

GameObject create_airplane(Gamestate gamestate, uint player)
{
	GameObject plane = create_game_object();
	// Set up the values for the opts
	// DRAW
	plane.draw.value = airplane_draw;
	plane.draw.has = 1;

	// initialize physics
	plane.physics.value = create_physics_component();
	plane.physics.has = 1;
	dGeomID geom;
	dBodyID body;
	dMass mass;

	body = dBodyCreate(gamestate.world);
	dMassSetBox(&mass, 1, AIRPLANE_DEBUG_CUBE_WIDTH, AIRPLANE_DEBUG_CUBE_WIDTH,
				AIRPLANE_DEBUG_CUBE_LENGTH);
	dMassAdjust(&mass, AIRPLANE_MASS);

	geom = dCreateBox(gamestate.space, AIRPLANE_DEBUG_CUBE_WIDTH,
					  AIRPLANE_DEBUG_CUBE_WIDTH, AIRPLANE_DEBUG_CUBE_LENGTH);
	dGeomSetBody(geom, body);

	plane.physics.value.body.value = body;
	plane.physics.value.body.has = 1;
	plane.physics.value.geom = geom;

	plane.physics.value.collision_handler.has = 0; // just a reminder this exist

	// UPDATE
	if (player == 0) {
		plane.update.value = &airplane_update_p1;
		plane.physics.value.bit = P1_PLANE_BIT;
		plane.physics.value.mask = P1_PLANE_MASK;
		dBodySetPosition(body, INITIAL_AIRPLANE_POS_P1);
	} else {
		plane.update.value = &airplane_update_p2;
		plane.physics.value.bit = P2_PLANE_BIT;
		plane.physics.value.mask = P2_PLANE_MASK;
		dBodySetPosition(body, INITIAL_AIRPLANE_POS_P2);
	}

	// Say that it has them
	plane.update.has = 1;

	return plane;
}

///
/// Code that should be run for both airplanes.
///
static void airplane_update_common(GameObject *self, Gamestate *gamestate,
								   float delta_time)
{
	UNUSED(delta_time);
	UNUSED(self);
	UNUSED(gamestate);
}

// Accept input state and move accordingly
static void airplane_update_p1(GameObject *self, Gamestate *gamestate,
							   float delta_time)
{
	UNUSED(delta_time);

	// apply foces based on inputs
	apply_airplane_input_impulses(self->physics.value.body.value,
								  gamestate->input.keys,
								  gamestate->input.controller);

	// set the camera to be at the location of the plane
	dBodyID body = self->physics.value.body.value;
	Vector3 pos = to_raylib(dBodyGetPosition(body));
	gamestate->p1_camera->position = pos;

	airplane_update_common(self, gamestate, delta_time);
}

static void airplane_update_p2(GameObject *self, Gamestate *gamestate,
							   float delta_time)
{
	UNUSED(delta_time);

	// apply foces based on inputs
	apply_airplane_input_impulses(self->physics.value.body.value,
								  gamestate->input.keys_2,
								  gamestate->input.controller_2);

	// set the camera to be at the location of the plane
	dBodyID body = self->physics.value.body.value;
	Vector3 pos = to_raylib(dBodyGetPosition(body));
	gamestate->p2_camera->position = pos;

	airplane_update_common(self, gamestate, delta_time);
}

// Draw the plane
static void airplane_draw(struct GameObject *self, Gamestate *gamestate)
{
	UNUSED(gamestate);
	dBodyID body = self->physics.value.body.value;
	DrawCube(to_raylib(dBodyGetPosition(body)), AIRPLANE_DEBUG_CUBE_WIDTH,
			 AIRPLANE_DEBUG_CUBE_WIDTH, AIRPLANE_DEBUG_CUBE_LENGTH,
			 AIRPLANE_DEBUG_CUBE_COLOR);
}

static void apply_airplane_input_impulses(dBodyID plane, Keystate keys,
										  ControllerState controls)
{
	// Get the current linear and angular velocity
	dVector3 *forward = dBodyGetLinearVel(plane);
	dBodyAddRelForce(plane, *forward[0], *forward[1], *forward[2]);

	// Check the state of the stick inputs (for your player index)
	// int controller_verti = controls.joystick.y;
	// int controller_hori = controls.joystick.x;

	//dBodySetAngularVel(plane, 100.0, 0.0, 0.0);	// if up/down, apply pitch
	// if (controller_verti > 0) {
	// }

	// if left/right, apply roll

	// Check the state of the keys (for your player index)
	// int key_hori = keys.left - keys.right;
	// if (key_hori > 0) {
	// 	dBodyAddRelTorque(plane, 100.0, 100.0, 100.0);
	// }
	// if lb/rb, apply yaw

	// Transform forward by the rotation matrix
	// Apply force on the transformed forward vector

	// Make this an impulse and apply it

	// --- COPIED PLAYER MOVEMENT CODE FOR YOUR REFERENCE ---
	// Vector3 impulse = {0};
	// Vector2 input = total_input(inputstate, player_index);

	// //impulse.x = sin(angle_x) * PLAYER_MOVE_IMPULSE;
	// //impulse.z = cos(angle_x) * PLAYER_MOVE_IMPULSE;
	// Vector3 h_impulse =
	// 	Vector3CrossProduct(Vector3Normalize(impulse), (Vector3){0, 1, 0});

	// impulse = Vector3Scale(impulse, input.y);

	// // also grab left/right input
	// h_impulse = Vector3Scale(h_impulse, input.x);

	// impulse = Vector3Add(impulse, h_impulse);

	//dBodyAddForce(plane, 1000.0, 000.0, 000.0);
}
