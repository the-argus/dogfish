#include "ode/ode.h"
#include "constants.h"
#include "physics.h"

static dWorldID world;
static dSpaceID space;
static dGeomID ground;
static dGeomID ground_box;

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
}

void close_physics()
{
	dSpaceDestroy(space);
	dWorldDestroy(world);
	dCloseODE();
}
