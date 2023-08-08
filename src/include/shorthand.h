#pragma once
#define UNUSED(expr) (void)(expr)
// this only works with clang and gcc, i think its better :(
// #define UNUSED(expr) __attribute__((unused)) expr

inline int sign(float value)
{
	if (value == 0) {
		return 0;
	}
	if (value > 0) {
		return 1;
	}
	return -1;
}
