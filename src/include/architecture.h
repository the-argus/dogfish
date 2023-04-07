#pragma once
#include "raylib.h"
#include "ode/ode.h"

#define OPTIONAL(type) \
typedef struct Opt_##type { \
    unsigned char has; \
    type func; \
} Opt_##type;

typedef struct
{
	Vector2 angle;			  // Camera angle in plane XZ
	int moveControl[6];	   // Move controls (CAMERA_FIRST_PERSON)
} CameraData;

typedef struct Mousestate {
    unsigned char left_pressed;
    unsigned char right_pressed;
    Vector2 position;
    Vector2 virtual_position;
} Mousestate;

typedef struct Keystate {
    unsigned char up;
    unsigned char left;
    unsigned char right;
    unsigned char down;
    unsigned char jump;
} Keystate;

typedef struct Inputstate {
    Mousestate mouse;
    Keystate keys;
} Inputstate;

typedef struct Gamestate {
    Inputstate input;
    Camera *current_camera;
    CameraData camera_data;
} Gamestate;

struct GameObject;

typedef void (*UpdateFunction)(Gamestate *gamestate, float delta_time);
typedef void (*DrawFunction)(Gamestate *gamestate);

// versions of types but they have a true/false for whether or not they
// are valid/populated
typedef dBodyID PhysicsBody;
OPTIONAL(UpdateFunction)
OPTIONAL(DrawFunction)
OPTIONAL(PhysicsBody)

// THE game object!!!!
typedef struct GameObject {
    Opt_PhysicsBody body;
    Opt_DrawFunction draw;
    Opt_UpdateFunction update;
} GameObject;
