#pragma once
#include "ode/ode.h"
#include "shorthand.h"

struct GameObject;
struct Gamestate;

#define OPTIONAL(type) \
typedef struct Opt_##type { \
    unsigned char has; \
    type value; \
} Opt_##type;

typedef void (*UpdateFunction)(struct GameObject *self, struct Gamestate *gamestate,
							   float delta_time);
typedef void (*DrawFunction)(struct GameObject *self, struct Gamestate *gamestate);
typedef void (*CollisionHandler)(dGeomID self, dGeomID other);
typedef void (*CleanupFunction)(struct GameObject *self);

// versions of types but they have a true/false for whether or not they
// are valid/populated
typedef dBodyID PhysicsBody;
typedef dGeomID PhysicsGeometry;

OPTIONAL(PhysicsBody)
OPTIONAL(CollisionHandler)

// a physics component always has a geom but not necessarily a body
typedef struct PhysicsComponent
{
	ushort mask; // 8 bits
	ushort bit;	 // 8 bits
	PhysicsGeometry geom;
	Opt_CollisionHandler collision_handler;
	Opt_PhysicsBody body; // this body's UserData should be a pointer to the
						  // parent gameobject
} PhysicsComponent;

OPTIONAL(UpdateFunction)
OPTIONAL(DrawFunction)
OPTIONAL(PhysicsComponent)
OPTIONAL(CleanupFunction)

// THE game object!!!!
// Possible implementations that will use game object:
//  - player/enemies (physics body determines in-world location, needs update
//  and draw)
//  - wall (has a physics GEOM, but no physics body)
//  - particle generator, effect, decorative model (no physics, no update, only
//  draw)
typedef struct GameObject
{
	Opt_PhysicsComponent physics;
	Opt_DrawFunction draw;
	Opt_UpdateFunction update;
	Opt_CleanupFunction cleanup;
	ushort id;
	uchar disabled : 1;
	uchar queued_for_cleanup : 1;
} GameObject;

///
/// Create an initialized GameObject, everything with 0. Basically a default
/// constructor- absolutely required for gameobject initialization.
///
GameObject create_game_object();

///
/// Create a physics component will all zero entries.
///
PhysicsComponent create_physics_component();
