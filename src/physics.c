#include "ode/ode.h"
#include "constants.h"
#include "physics.h"
#include "raylib.h"

static dWorldID world;
static dSpaceID space;
static dGeomID ground;

static dBodyID test_cube;
static dGeomID test_cube_geom;
static const float test_cube_mass = 1;
static dJointGroupID contactgroup;

Vector3 to_raylib(const dVector3 v3) { return (Vector3){v3[0], v3[1], v3[2]}; }
const dReal *get_test_cube_position() { return dBodyGetPosition(test_cube); }
const Vector3 get_test_cube_size()
{
	const Vector3 size = {1, 1, 1};
	return size;
}

static void nearCallback(void *unused, dGeomID o1, dGeomID o2)
{
	int i;

	// only collide things with the ground
	int g1 = (o1 == ground);
	int g2 = (o2 == ground);
	if (!(g1 ^ g2))
		return;

	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);

	dContact contact[3]; // up to 3 contacts per box
	for (i = 0; i < 3; i++) {
		contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
		contact[i].surface.mu = 0.5;
		contact[i].surface.soft_cfm = 0.01;
	}
	int numc = dCollide(o1, o2, 3, &contact[0].geom, sizeof(dContact));
	for (i = 0; i < numc; i++) {
		dJointID c = dJointCreateContact(world, contactgroup, contact + i);
		dJointAttach(c, b1, b2);
	}
}

void update_physics(float delta_time)
{
	dSpaceCollide(space, 0, nearCallback);
	dWorldStep(world, delta_time);
	dJointGroupEmpty(contactgroup);
}

void init_physics()
{
	dInitODE();
	world = dWorldCreate();
	space = dHashSpaceCreate(0);
	dWorldSetGravity(world, 0, -GRAVITY, 0);

	contactgroup = dJointGroupCreate(0);

	ground = dCreatePlane(space, 0, 1, 0, 0);

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

	// make geometry and apply it to the cube body
	test_cube_geom = dCreateBox(space, cube_size.x, cube_size.y, cube_size.z);
	dGeomSetBody(test_cube_geom, test_cube);
}

void close_physics()
{
	dJointGroupDestroy(contactgroup);
	dSpaceDestroy(space);
	dWorldDestroy(world);
	dCloseODE();
}
