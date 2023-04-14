#include "airplane.h"
#include "include/constants.h"
#include "physics.h"
#include "shorthand.h"
#include "gameobject.h"
#include "raymath.h"
#include "debug.h"
#include "bullet.h"
#include "object_structure.h"

#define AIRPLANE_DEBUG_CUBE_WIDTH 0.5
#define AIRPLANE_DEBUG_CUBE_LENGTH 2
#define AIRPLANE_DEBUG_CUBE_COLOR RED
#define AIRPLANE_MASS 5.4
#define INITIAL_AIRPLANE_POS_P1 5, 10, 0
#define INITIAL_AIRPLANE_POS_P2 -5, 10, 0

#define CAMERA_FIRST_PERSON_MIN_CLAMP -1.0f
#define CAMERA_FIRST_PERSON_MAX_CLAMP -179.0f
#define CAMERA_MOUSE_MOVE_SENSITIVITY 0.5f

static Model p1_model;
static Model p2_model;

static void airplane_update_p1(struct GameObject *self, Gamestate *gamestate,
							   float delta_time);
static void airplane_update_p2(struct GameObject *self, Gamestate *gamestate,
							   float delta_time);
static void apply_airplane_input_impulses(dBodyID plane, Keystate keys,
										  ControllerState controls);
static void airplane_draw_p1(struct GameObject *self, Gamestate *gamestate);
static void airplane_draw_p2(struct GameObject *self, Gamestate *gamestate);
static void airplane_cleanup_p1(struct GameObject *self);
static void airplane_cleanup_p2(struct GameObject *self);
static int sign(float value);

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

	dBodySetLinearDamping(body, 0.01f);
	dBodySetAngularDamping(body, 0.0f);

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
								   float delta_time, int shooting)
{
	UNUSED(delta_time);
	UNUSED(gamestate);

	if (shooting) {
		dBodyID body = self->physics.value.body.value;
		Vector3 pos = to_raylib(dBodyGetPosition(body));
		Vector3 forward = to_raylib(dBodyGetLinearVel(body));
		forward = Vector3Normalize(forward);
		forward = Vector3Scale(forward, 10);

		GameObject bullet = create_bullet(*gamestate);
		dBodyID bullet_body = bullet.physics.value.body.value;
		dBodySetLinearVel(bullet_body, forward.x, forward.y, forward.z);
		dBodySetPosition(bullet_body, pos.x, pos.y, pos.z);
		object_structure_queue_for_creation(gamestate->objects, bullet);
	}
}

// Accept input state and move accordingly
static void airplane_update_p1(GameObject *self, Gamestate *gamestate,
							   float delta_time)
{
	// apply foces based on inputs
	apply_airplane_input_impulses(self->physics.value.body.value,
								  gamestate->input.keys,
								  gamestate->input.controller);

	// set the camera to be at the location of the plane
#ifdef DEBUG_CAMERA
	UseDebugCameraController(gamestate->p1_camera);
#else
	dBodyID body = self->physics.value.body.value;
	Vector3 pos = to_raylib(dBodyGetPosition(body));
	gamestate->p1_camera->target = pos; // look at the plane
	Vector3 camera_diff = {0, 5, 0};
	// rotate by angles x and y
	gamestate->p1_camera_data.angle.x -=
		(gamestate->input.cursor.delta.x +
		 gamestate->input.controller.joystick.x) *
		CAMERA_MOUSE_MOVE_SENSITIVITY * delta_time;
	gamestate->p1_camera_data.angle.y -=
		(gamestate->input.cursor.delta.y +
		 gamestate->input.controller.joystick.y) *
		CAMERA_MOUSE_MOVE_SENSITIVITY * delta_time;
	// clamp y
	if (gamestate->p1_camera_data.angle.y >
		CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD) {
		gamestate->p1_camera_data.angle.y =
			CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD;
	} else if (gamestate->p1_camera_data.angle.y <
			   CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD) {
		gamestate->p1_camera_data.angle.y =
			CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD;
	}
	// clamp x
	gamestate->p1_camera_data.angle.x -=
		((int)(gamestate->p1_camera_data.angle.x / (2 * PI))) * (2 * PI);

	// apply rotation to camera_diff
	Quaternion camera_rot =
		QuaternionFromEuler(gamestate->p1_camera_data.angle.y,
							gamestate->p1_camera_data.angle.x, 0);
	camera_diff = Vector3RotateByQuaternion(camera_diff, camera_rot);
	gamestate->p1_camera->position = Vector3Add(pos, camera_diff);
#endif

	airplane_update_common(self, gamestate, delta_time,
						   gamestate->input.cursor.shoot ||
							   gamestate->input.controller.shoot);
}

static void airplane_update_p2(GameObject *self, Gamestate *gamestate,
							   float delta_time)
{
	// apply foces based on inputs
	apply_airplane_input_impulses(self->physics.value.body.value,
								  gamestate->input.keys_2,
								  gamestate->input.controller_2);

	// set the camera to be at the location of the plane
	dBodyID body = self->physics.value.body.value;
	Vector3 pos = to_raylib(dBodyGetPosition(body));
	gamestate->p2_camera->target = pos;

	airplane_update_common(self, gamestate, delta_time,
						   gamestate->input.controller_2.shoot);
}

// Draw the p1 model at the p1 position
static void airplane_draw_p1(struct GameObject *self, Gamestate *gamestate)
{
	UNUSED(gamestate);

	dBodyID body = self->physics.value.body.value;

	// Tranformation matrix for rotations
	Vector3 plane_rotation = to_raylib(dBodyGetRotation(body));
	p1_model.transform = MatrixRotateXYZ(
		(Vector3){plane_rotation.x, plane_rotation.y, plane_rotation.z});
	DrawModel(p1_model, to_raylib(dBodyGetPosition(body)), 1.0, BLUE);
}

// Draw the p2 model at the p2 position
static void airplane_draw_p2(struct GameObject *self, Gamestate *gamestate)
{
	UNUSED(gamestate);

	dBodyID body = self->physics.value.body.value;

	// Tranformation matrix for rotations
	Vector3 plane_rotation = to_raylib(dBodyGetRotation(body));
	p2_model.transform = MatrixRotateXYZ(
		(Vector3){plane_rotation.x, plane_rotation.y, plane_rotation.z});
	DrawModel(p2_model, to_raylib(dBodyGetPosition(body)), 1.0, BLUE);
}

static void airplane_cleanup_p1(struct GameObject *self)
{
	UnloadModel(p1_model);
}

static void airplane_cleanup_p2(struct GameObject *self)
{
	UnloadModel(p2_model);
}

static int sign(float value)
{
	if (value == 0) {
		return 0;
	} else if (value > 0) {
		return 1;
	} else {
		return -1;
	}
}

static void apply_airplane_input_impulses(dBodyID plane, Keystate keys,
										  ControllerState controls)
{
	// Get the current linear and angular velocity
	Vector3 forward = to_raylib(dBodyGetLinearVel(plane));
	forward = Vector3Normalize(forward);

	Vector3 impulse = {0, -1 * GRAVITY, 0};
	Vector3 torque_impulse = {0, 0, 0};

	int vertical_input = sign(keys.up - keys.down + sign(controls.joystick.y));
	int horizontal_input =
		sign(keys.right - keys.left + sign(controls.joystick.x));

	int thrust = controls.boost || keys.boost;

	// modify impulse and torque based on input

	// first, get forwards movement (along the direction of the plane)
	Vector3 movement =
		Vector3Scale(forward, thrust ? PLANE_BOOST_SPEED : PLANE_MOVE_SPEED);
	impulse = Vector3Add(impulse, movement);

	torque_impulse.y += 0.1f * horizontal_input;
	torque_impulse.z += 0.1f * vertical_input;

	dBodySetForce(plane, impulse.x, impulse.y, impulse.z);
	dBodySetTorque(plane, torque_impulse.x, torque_impulse.y, torque_impulse.z);
}
