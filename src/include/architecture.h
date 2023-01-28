#pragma once

struct Mousestate {
    unsigned char left_pressed;
    unsigned char right_pressed;
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
