#pragma once
#include "raylib.h"

struct Mousestate {
    unsigned char left_pressed;
    unsigned char right_pressed;
    Vector2 position;
    Vector2 virtual_position;
};

struct Keystate {
    unsigned char up;
    unsigned char left;
    unsigned char right;
    unsigned char down;
};

struct Inputstate {
    struct Mousestate mouse;
    struct Keystate keys;
};

struct Gamestate {
    struct Inputstate input;
    Camera current_camera;
};
