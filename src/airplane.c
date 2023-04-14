#include "airplane.h"
#include "include/constants.h"
#include "physics.h"
#include "shorthand.h"
#include "gameobject.h"
#include "raymath.h"

#define AIRPLANE_DEBUG_CUBE_WIDTH 0.5
#define AIRPLANE_DEBUG_CUBE_LENGTH 2
#define AIRPLANE_DEBUG_CUBE_COLOR RED
#define AIRPLANE_MASS 5.4
#define INITIAL_AIRPLANE_POS_P1 5, 10, 0
#define INITIAL_AIRPLANE_POS_P2 -5, 10, 0

static Model p1_model;
static Model p2_model;

// Accept p1's input state from game state, and move accordingly
static void airplane_update_p1(struct GameObject *self, Gamestate *gamestate,
							   float delta_time);

// Accept p2's input state from game state, and move accordingly
static void airplane_update_p2(struct GameObject *self, Gamestate *gamestate,
							   float delta_time);

// general function that applies airplane-like forces based on keys and controls
static void apply_airplane_input_impulses(dBodyID plane, Keystate keys,
										  ControllerState controls);

// Draw p1 plane
static void airplane_draw_p1(struct GameObject *self, Gamestate *gamestate);

// Draw p2 plane
static void airplane_draw_p2(struct GameObject *self, Gamestate *gamestate);

// Cleanup p1
static void airplane_cleanup_p1(struct GameObject *self);

// Cleanup p2
static void airplane_cleanup_p2(struct GameObject *self);

GameObject create_airplane(Gamestate gamestate, uint player)
{
	GameObject plane = create_game_object();
	// Set up the values for the opts

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
		plane.draw.value = &airplane_draw_p1;
		plane.cleanup.value = &airplane_cleanup_p1;
		plane.physics.value.bit = P1_PLANE_BIT;
		plane.physics.value.mask = P1_PLANE_MASK;
		p1_model = LoadModelFromMesh(GenMeshCube(2.0f, 1.0f, 2.0f));
		dBodySetPosition(body, INITIAL_AIRPLANE_POS_P1);
	} else {
		plane.update.value = &airplane_update_p2;
		plane.draw.value = &airplane_draw_p2;
		plane.cleanup.value = &airplane_cleanup_p2;
		plane.physics.value.bit = P2_PLANE_BIT;
		plane.physics.value.mask = P2_PLANE_MASK;
		p2_model = LoadModelFromMesh(GenMeshCube(2.0f, 1.0f, 2.0f));
		dBodySetPosition(body, INITIAL_AIRPLANE_POS_P2);
	}

	// Say that it has them
	plane.update.has = 1;
	plane.draw.has = 1;
	plane.cleanup.has = 1;

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

// Draw the p1 model at the p1 position
static void airplane_draw_p1(struct GameObject *self, Gamestate *gamestate)
{
	UNUSED(gamestate);
	
	dBodyID body = self->physics.value.body.value;

	// Tranformation matrix for rotations
	Vector3 plane_rotation = to_raylib(dBodyGetRotation(body));
	p1_model.transform = MatrixRotateXYZ((Vector3){DEG2RAD * plane_rotation.x, DEG2RAD * plane_rotation.y, DEG2RAD * plane_rotation.z});
	DrawModel(p1_model, to_raylib(dBodyGetPosition(body)), 1.0, BLUE);
	//UnloadModel(planemodel);
}

// Draw the p2 model at the p2 position
static void airplane_draw_p2(struct GameObject *self, Gamestate *gamestate)
{
	UNUSED(gamestate);
	
	dBodyID body = self->physics.value.body.value;

	// Tranformation matrix for rotations
	Vector3 plane_rotation = to_raylib(dBodyGetRotation(body));
	p2_model.transform = MatrixRotateXYZ((Vector3){DEG2RAD * plane_rotation.x, DEG2RAD * plane_rotation.y, DEG2RAD * plane_rotation.z});
	DrawModel(p2_model, to_raylib(dBodyGetPosition(body)), 1.0, BLUE);
	//UnloadModel(planemodel);
}

static void airplane_cleanup_p1(struct GameObject *self)
{
	UnloadModel(p1_model);
}

static void airplane_cleanup_p2(struct GameObject *self)
{
	UnloadModel(p2_model);
}

static void apply_airplane_input_impulses(dBodyID plane, Keystate keys,
										  ControllerState controls)
{
	// Get the current linear and angular velocity
	Vector3 forward = to_raylib(dBodyGetLinearVel(plane));
	//dBodyAddRelForce(plane, forward.x, forward.y, forward.z);

	// attempt to counteract gravity, doesn't work
	// overwrites the add rel force above
	dBodyAddForce(plane, 0.0, 0.5, 0.0);

	// Check the state of the stick inputs (for your player index)
	int controller_verti = controls.joystick.y;
	int controller_hori = controls.joystick.x;

	// dBodySetAngularVel(plane, 100.0, 0.0, 0.0);	// if up/down, apply pitch
	if (controller_verti > 0) { // stick down, pull up
		//printf('a');
		//dBodyAddRelTorque(plane, 0.0, 100.0, 0.0);
		dBodyAddRelForce(plane, forward.x, -100.0, forward.z);
		//dBodySetAngularVel (plane, 100.0, 100.0, 0.0);
		//dBodySetRotation (dBodyID, const dMatrix3 R);
	} else if (controller_verti < 0) { // stick up, pull down
		//dBodyAddRelTorque(plane, 0.0, -100.0, 0.0);
		dBodyAddRelForce(plane, forward.x, 100.0, forward.z);
	}

	// if left/right, apply roll
	if (controller_hori > 0) {
		dBodyAddRelTorque(plane, 0.0, 0.0, 10.0);
	} else if (controller_hori < 0) {
		dBodyAddRelTorque(plane, 0.0, 0.0, -10.0);
	}

	// Check the state of the keys (for your player index)
	int key_hori = keys.left - keys.right;
	if (key_hori > 0) {
		//dBodyAddRelTorque(plane, 100.0, 100.0, 100.0);
	}
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

	// dBodyAddForce(plane, 1000.0, 000.0, 000.0);
}
