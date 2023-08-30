#pragma once
#include <raylib.h>
#include <stddef.h>

typedef struct
{
	Mesh inner;
	size_t triangle_index;
	size_t vert_index;
	Vector2 uv;
	Vector3 normal;
#ifndef NDEBUG
	bool allocated;
#endif
} Mesher;

/// Initializes a mesher. Does not perform memory allocations
void mesher_create(Mesher*);

/// Allocates the vertex, normal, and texcoord buffers for a mesher.
void mesher_allocate(Mesher* mesher, size_t quads);

/// Add a vertex to the mesh
void mesher_push_vertex(Mesher* mesher, const Vector3* offset,
						const Vector3* vertex);

/// Returns the mesh and resets the mesher. Needs reallocation after this
Mesh mesher_release(Mesher* mesher);
