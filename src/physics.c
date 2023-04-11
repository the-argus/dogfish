#include "input.h"
#include "ode/ode.h"
#include "constants.h"
#include "architecture.h"
#include "physics.h"
#include "raylib.h"
#include "raymath.h"

#include "debug.h"

static dWorldID world;
static dSpaceID space;
static dGeomID ground;

static dBodyID test_cube;
static dGeomID test_cube_geom;
static const float test_cube_mass = 1;
static dJointGroupID contactgroup;

Vector3 to_raylib(const dVector3 v3) { return (Vector3){v3[0], v3[1], v3[2]}; }
const dReal *get_test_cube_position() { return dBodyGetPosition(test_cube); }
Vector3 get_test_cube_size()
{
	const Vector3 size = {1, 1, 1};
	return size;
}

static void init_contact(dContact *contact)
{
	contact->surface.mode = 0;
	contact->surface.mu = 0.5;
	contact->surface.mu2 = 0;
}

static int on_ground(dBodyID body)
{
	dGeomID geom = dBodyGetFirstGeom(body);
	dContact contact;
	init_contact(&contact);

	if (dCollide(geom, ground, 1, &contact.geom, sizeof(dContactGeom))) {
		// ensure the collision was with a face which is pointing up
		float upness = Vector3DotProduct(to_raylib(contact.geom.normal),
										 (Vector3){0, 1, 0});
		return upness > ON_GROUND_THRESHHOLD;
	}
	return 0;
}

void apply_player_input_impulses(Inputstate inputstate, float angle_x)
{
	Vector3 impulse = {0};
    Vector2 input = total_input(inputstate, 0);

	impulse.x = sin(angle_x) * PLAYER_MOVE_IMPULSE;
	impulse.z = cos(angle_x) * PLAYER_MOVE_IMPULSE;
	Vector3 h_impulse =
		Vector3CrossProduct(Vector3Normalize(impulse), (Vector3){0, 1, 0});

	impulse = Vector3Scale(impulse, input.y);

	// also grab left/right input
	h_impulse = Vector3Scale(h_impulse, input.x);

	impulse = Vector3Add(impulse, h_impulse);

	dBodyAddForce(test_cube, impulse.x, impulse.y, impulse.z);
}

void apply_airplane_input_impulses(dBodyID plane, dBodyID ground, Inputstate inputstate, unsigned int player_index)
{
	dReal *planeLinear[3] = dBodyGetLinearVel( plane );
	
	Vector3 impulse = {0};
    Vector2 input = total_input(inputstate, player_index);

	//impulse.x = sin(angle_x) * PLAYER_MOVE_IMPULSE;
	//impulse.z = cos(angle_x) * PLAYER_MOVE_IMPULSE;
	Vector3 h_impulse =
		Vector3CrossProduct(Vector3Normalize(impulse), (Vector3){0, 1, 0});

	impulse = Vector3Scale(impulse, input.y);

	// also grab left/right input
	h_impulse = Vector3Scale(h_impulse, input.x);

	impulse = Vector3Add(impulse, h_impulse);

	

	dBodyAddForce(test_cube, impulse.x, impulse.y, impulse.z);
}

static void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
	UNUSED(data);

	// only collide things with the ground
	int g1 = (o1 == ground);
	int g2 = (o2 == ground);
	if (!(g1 ^ g2))
		return;

	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);

	if (b1 && b2 && dAreConnected(b1, b2))
		return;

	dContact contact;
	init_contact(&contact);
	if (dCollide(o1, o2, 1, &contact.geom, sizeof(contact.geom))) {
		dJointID c = dJointCreateContact(world, contactgroup, &contact);
		dJointAttach(c, b1, b2);
	}
}

void update_physics(float delta_time)
{
	dSpaceCollide(space, 0, &nearCallback);
	dWorldStep(world, delta_time);
	dJointGroupEmpty(contactgroup);
}

void init_physics(Gamestate *gamestate)
{
	dInitODE2(0);

	// create world and space, send them to the gamestate
	world = dWorldCreate();
	space = dHashSpaceCreate(0);
	gamestate->world = world;
	gamestate->space = space;

	contactgroup = dJointGroupCreate(0);
	dWorldSetGravity(world, 0, -GRAVITY, 0);
	ground = dCreatePlane(space, 0, 1, 0, 0);

	#pragma region CUBE

	// create a cube which raylib can draw later
	test_cube = dBodyCreate(world);
	dBodySetPosition(test_cube, 0, 5, 0);

	// set up the mass
	dMass cube_mass;
	Vector3 cube_size = get_test_cube_size();
	dMassSetBox(&cube_mass, 1, cube_size.x, cube_size.y, cube_size.z);
	dMassAdjust(&cube_mass, test_cube_mass);

	// apply the mass to the cube
	dBodySetMass(test_cube, &cube_mass);

	// infinite moment of inertia, basically
	dBodySetAngularDamping(test_cube, 1);

	// make geometry and apply it to the cube body
	test_cube_geom = dCreateBox(space, cube_size.x, cube_size.y, cube_size.z);
	dGeomSetBody(test_cube_geom, test_cube);

	#pragma endregion

	#pragma region PLANE1

	#pragma endregion

	// allocate the data for this thread to access ODE
	assert(dAllocateODEDataForThread(dAllocateMaskAll));
}

void close_physics()
{
	dCleanupODEAllDataForThread();
	dJointGroupDestroy(contactgroup);
	dSpaceDestroy(space);
	dWorldDestroy(world);
	dCloseODE();
}
