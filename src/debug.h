#pragma once
#include <raylib.h>
#include <stdint.h>

void Vector3ToString(char *buffer, uint32_t size, Vector3 vector);
void Vector3Print(Vector3 vector, const char *name);
void UseDebugCameraController(Camera *camera_to_move);
