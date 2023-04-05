#pragma once
#include "raylib.h"
#include "fps_camera.h"

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
} Keystate;

typedef struct Inputstate {
    struct Mousestate mouse;
    struct Keystate keys;
} Inputstate;

typedef struct Gamestate {
    struct Inputstate input;
    Camera *current_camera;
    CameraData camera_data;
} Gamestate;
