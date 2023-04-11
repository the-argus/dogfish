#pragma once
#include "raylib.h"
#include "ode/ode.h"
#include "shorthand.h"

#define OPTIONAL(type) \
typedef struct Opt_##type { \
    unsigned char has; \
    type value; \
} Opt_##type;

typedef struct
{
	Vector2 angle;			  // Camera angle in plane XZ
} CameraData;

typedef struct Cursorstate {
    Vector2 position;
    Vector2 virtual_position;
    Vector2 delta;
} Cursorstate;

typedef struct Keystate {
    unsigned char up : 1;
    unsigned char left : 1;
    unsigned char right : 1;
    unsigned char down : 1;
    unsigned char look_left : 1;
    unsigned char look_right : 1;
    unsigned char look_up : 1;
    unsigned char look_down : 1;
} Keystate;

typedef Vector2 Joystick;

typedef struct ControllerState {
    unsigned char left : 1;
    unsigned char up : 1;
    unsigned char down : 1;
    unsigned char right : 1;
    Joystick joystick;
} ControllerState;

typedef struct Inputstate {
    Cursorstate cursor;
    Cursorstate cursor_2;
    Keystate keys;
    Keystate keys_2;
    ControllerState controller;
    ControllerState controller_2;
} Inputstate;

typedef struct Gamestate {
    Inputstate input;
    Camera *p1_camera;
    CameraData p1_camera_data;
    Camera *p2_camera;
    CameraData p2_camera_data;
} Gamestate;

struct GameObject;

typedef void (*UpdateFunction)(struct GameObject *self, Gamestate *gamestate, float delta_time);
typedef void (*DrawFunction)(struct GameObject *self, Gamestate *gamestate);
typedef void (*CollisionHandler)(dGeomID self, dGeomID other);

// versions of types but they have a true/false for whether or not they
// are valid/populated
typedef dBodyID PhysicsBody;
typedef dGeomID PhysicsGeometry;

OPTIONAL(PhysicsBody)
OPTIONAL(CollisionHandler)

// a physics component always has a geom but not necessarily a body
typedef struct PhysicsComponent {
    unsigned short mask; // 8 bits
    unsigned short bit; // 8 bits
    PhysicsGeometry geom;
    Opt_CollisionHandler collision_handler;
    Opt_PhysicsBody body; // this body's UserData should be a pointer to the parent gameobject
} PhysicsComponent;

OPTIONAL(UpdateFunction)
OPTIONAL(DrawFunction)
OPTIONAL(PhysicsComponent)

// THE game object!!!!
// Possible implementations that will use game object:
//  - player/enemies (physics body determines in-world location, needs update and draw)
//  - wall (has a physics GEOM, but no physics body)
//  - particle generator, effect, decorative model (no physics, no update, only draw)
typedef struct GameObject {
    Opt_PhysicsComponent physics;
    Opt_DrawFunction draw;
    Opt_UpdateFunction update;
    u_int16_t id;
    u_int8_t disabled: 1;
    u_int8_t queued_for_cleanup: 1;
} GameObject;
