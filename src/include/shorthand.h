#pragma once
#define voidptr void *

#define UNUSED(expr) (void)(expr)
// this only works with clang and gcc, i think its better :(
// #define UNUSED(expr) __attribute__((unused)) expr
