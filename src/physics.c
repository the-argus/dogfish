#include "ode/ode.h"
#include "constants.h"
#include "physics.h"

static dWorldID world;
static dSpaceID space;
// static dGeomID ground;

void init_physics()
{
	dInitODE();
	world = dWorldCreate();
    space = dHashSpaceCreate(0);
    dWorldSetGravity(world, 0, 0, -GRAVITY);
}

void close_physics() { dCloseODE(); }
