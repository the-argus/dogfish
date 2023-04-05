#pragma once
#include "raylib.h"
#include "architecture.h"

void fps_camera_update(Camera *camera, CameraData *camera_data);
void update_camera_tilt(Camera *camera, Inputstate input);
