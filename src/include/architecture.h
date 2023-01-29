#pragma once
#include "raylib.h"

struct Mousestate {
    unsigned char left_pressed;
    unsigned char right_pressed;
    Vector2 position;
    Vector2 virtual_position;
};

struct Keystate {
    unsigned char w;
    unsigned char a;
    unsigned char s;
    unsigned char d;
};

struct Inputstate {
    struct Mousestate mouse;
    struct Keystate keys;
};

struct Gamestate {
    struct Inputstate input;
};
