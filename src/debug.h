#pragma once
#include "raylib.h"
#include "shorthand.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void Vector3ToString(char *buffer, uint size, Vector3 vector);
void Vector3Print(Vector3 vector, const char *name);
void UseDebugCameraController(Camera* camera_to_move);
