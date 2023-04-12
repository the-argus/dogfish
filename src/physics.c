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
static dGeomID ground; // accessible only here, exposed only by the on_ground
					   // function
static dJointGroupID contactgroup;

Vector3 to_raylib(const dVector3 v3) { return (Vector3){v3[0], v3[1], v3[2]}; }

static void init_contact(dContact *contact)
{
	contact->surface.mode = 0;
	contact->surface.mu = 0.5;
	contact->surface.mu2 = 0;
}

int on_ground(dBodyID body)
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
		CollisionHandler handler1 = (CollisionHandler)dGeomGetData(o1);
		CollisionHandler handler2 = (CollisionHandler)dGeomGetData(o1);

		if (handler1 != NULL) {
			handler1(o1, o2);
		}
		if (handler2 != NULL) {
			handler1(o2, o1);
		}
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
