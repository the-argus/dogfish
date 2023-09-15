#pragma once
#include <raylib.h>

void UploadTerrainMesh(Mesh* mesh, const Mesh* existing_mesh, bool dynamic);

/// Set VAOs and VBOs to 0 so as to avoid unloading them upon UnloadMesh
void terrain_render_clear_mesh(Mesh* mesh);
