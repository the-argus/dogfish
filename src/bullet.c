#include "bullet.h"
#include "constants.h"
#include "physics.h"
#include "shorthand.h"
#include <raylib.h>

#define BULLET_WIDTH 0.1f
#define BULLET_LENGTH 0.5f
#define BULLET_MASS 1

static void bullet_update(GameObject *self, Gamestate *gamestate,
						  float delta_time);
static void bullet_draw(GameObject *self, Gamestate *gamestate);

GameObject create_bullet(Gamestate gamestate)
{
	GameObject initial_bullet = create_game_object();

	initial_bullet.draw.value = bullet_draw;
	initial_bullet.draw.has = 1;

	initial_bullet.update.value = bullet_update;
	initial_bullet.update.has = 1;

	initial_bullet.physics.has = 1;
	initial_bullet.physics.value = create_physics_component();
	initial_bullet.physics.value.mask = BULLET_MASK;
	initial_bullet.physics.value.bit = BULLET_BIT;
	// TODO: set up later
	dBodyID body = dBodyCreate(gamestate.world);
	initial_bullet.physics.value.body.value = body;

	dMass mass;
	Vector3 size = {BULLET_LENGTH, BULLET_WIDTH, BULLET_WIDTH};
	dMassSetBox(&mass, 1, size.x, size.y, size.z);
	dMassAdjust(&mass, BULLET_MASS);

	dBodySetPosition(body, 0, 1, 0);

	// apply the mass to the cube
	dBodySetMass(body, &mass);

	// infinite moment of inertia, basically
	dBodySetAngularDamping(body, 1);

	// make geometry and apply it to the cube body
	dGeomID geom = dCreateBox(gamestate.space, size.x, size.y, size.z);
	dGeomSetBody(geom, body);

	initial_bullet.physics.value.body.has = 1;

	return initial_bullet;
}

static void bullet_update(GameObject *self, Gamestate *gamestate,
						  float delta_time)
{
	UNUSED(self);
	UNUSED(gamestate);
	UNUSED(delta_time);
}

static void bullet_draw(GameObject *self, Gamestate *gamestate)
{
	UNUSED(gamestate);
	Vector3 pos = to_raylib(dBodyGetPosition(self->physics.value.body.value));
	DrawCube(pos, BULLET_LENGTH, BULLET_WIDTH, BULLET_WIDTH, WHITE);
}
