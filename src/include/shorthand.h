#pragma once
#define voidptr void *
typedef unsigned int uint;
typedef unsigned char uchar;

#define UNUSED(expr) (void)(expr)
// this only works with clang and gcc, i think its better :(
// #define UNUSED(expr) __attribute__((unused)) expr
