#include "ode/ode.h"
#include "constants.h"
#include "physics.h"
#include "raylib.h"

static dWorldID world;
static dSpaceID space;
static dGeomID ground;
static dGeomID ground_box;

static dBodyID test_cube;
static dGeomID test_cube_geom;
static const float test_cube_mass = 1;

const dReal *get_test_cube_position() { return dBodyGetPosition(test_cube); }
const Vector3 get_test_cube_size()
{
	const Vector3 size = {1, 1, 1};
	return size;
}

static void nearCallback(void *unused, dGeomID o1, dGeomID o2) { return; }

void update_physics(float delta_time)
{
	dSpaceCollide(space, 0, nearCallback);
	dWorldStep(world, delta_time);
}

void init_physics()
{
	dInitODE();
	world = dWorldCreate();
	space = dHashSpaceCreate(0);
	dWorldSetGravity(world, 0, 0, -GRAVITY);

	// TODO - figure out what the a b c d arguments here are even for
	ground = dCreatePlane(space, 0, 0, 1, 0);
	ground_box = dCreateBox(space, 10, 10, 1);
	dGeomSetPosition(ground_box, -5, -5, 0);

	// rotation of ground box
	dMatrix3 R;
	dRFromAxisAndAngle(R, 0, 1, 1, -0.15);
	dGeomSetRotation(ground_box, R);

	// create a cube which raylib can draw later
	test_cube = dBodyCreate(world);
	dBodySetPosition(test_cube, 0, 0, 5);

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
	dSpaceDestroy(space);
	dWorldDestroy(world);
	dCloseODE();
}
