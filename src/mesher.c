#include "mesher.h"
#include <assert.h>
#include <stdlib.h>

#define VERTEX_PER_QUAD 6
#define TRI_PER_QUAD 2

/// Initializes a mesher. Does not perform memory allocations
void mesher_create(Mesher* mesher)
{
	mesher->inner = (Mesh){0};
	mesher->vert_index = 0;
	mesher->triangle_index = 0;
	mesher->uv = (Vector2){0};
	mesher->normal = (Vector3){0};

	// TODO: figure out if this is necessary or (Mesh){0} covers it
	mesher->inner.vertices = NULL;
	mesher->inner.normals = NULL;
	mesher->inner.texcoords = NULL;
	mesher->inner.colors = NULL;
	mesher->inner.animNormals = NULL;
	mesher->inner.animVertices = NULL;
	mesher->inner.boneIds = NULL;
	mesher->inner.boneWeights = NULL;
	mesher->inner.tangents = NULL;
	mesher->inner.indices = NULL;
	mesher->inner.texcoords2 = NULL;
	mesher->inner.vboId = 0;
	mesher->inner.vaoId = 0;
#ifndef NDEBUG
	mesher->allocated = false;
#endif
}

/// Allocates the vertex, normal, and texcoord buffers for a mesher.
void mesher_allocate(Mesher* mesher, size_t quads)
{
	assert(!mesher->allocated);
	mesher->inner.vertexCount = (int)quads * VERTEX_PER_QUAD;
	mesher->inner.triangleCount = (int)quads * TRI_PER_QUAD;
	if (quads == 0) {
		return;
	}
#ifndef NDEBUG
	mesher->allocated = true;
	mesher->mesh_normal_float_indices = 3 * mesher->inner.vertexCount;
	mesher->mesh_vertex_float_indices = 3 * mesher->inner.vertexCount;
	mesher->mesh_texcoord_float_indices = 2 * mesher->inner.vertexCount;
#endif

	mesher->inner.vertices =
		RL_MALLOC(sizeof(float) * 3 * mesher->inner.vertexCount);
	mesher->inner.normals =
		RL_MALLOC(sizeof(float) * 3 * mesher->inner.vertexCount);
	mesher->inner.texcoords =
		RL_MALLOC(sizeof(float) * 2 * mesher->inner.vertexCount);
	mesher->inner.colors = NULL;
#ifndef NDEBUG
	mesher->allocated = true;
#endif
}

/// Add a vertex to the mesh
void mesher_push_vertex(Mesher* mesher, const Vector3* offset,
						const Vector3* vertex)
{
	if (mesher->inner.vertexCount == 0) {
		return;
	}
	assert(mesher->inner.texcoords != NULL);
	{
		size_t index = mesher->triangle_index * 6 + mesher->vert_index * 2;
		if (!(index < mesher->mesh_texcoord_float_indices)) {
			// TraceLog(LOG_INFO, "Index %d exceeds number of texcoords %d", index,
			// 		 mesher->mesh_texcoord_float_indices);
			return;
		}
		assert(index < mesher->mesh_texcoord_float_indices);
		mesher->inner.texcoords[index] = mesher->uv.x;
		mesher->inner.texcoords[index + 1] = mesher->uv.y;
	}

	assert(mesher->inner.normals != NULL);
	{
		size_t index = mesher->triangle_index * 9 + mesher->vert_index * 3;
		assert(index < mesher->mesh_normal_float_indices);
		mesher->inner.normals[index] = mesher->normal.x;
		mesher->inner.normals[index + 1] = mesher->normal.y;
		mesher->inner.normals[index + 2] = mesher->normal.z;
	}

	assert(mesher->inner.vertices != NULL);
	{
		size_t index = mesher->triangle_index * 9 + mesher->vert_index * 3;
		const Vector3 real_offset = offset ? *offset : (Vector3){0};
		assert(index < mesher->mesh_vertex_float_indices);
		mesher->inner.vertices[index] = vertex->x + real_offset.x;
		mesher->inner.vertices[index + 1] = vertex->y + real_offset.y;
		mesher->inner.vertices[index + 2] = vertex->z + real_offset.z;
	}

	++mesher->vert_index;
	if (mesher->vert_index > 2) {
		++mesher->triangle_index;
		mesher->vert_index = 0;
	}
}

Mesh mesher_release(Mesher* mesher)
{
	const Mesh result = mesher->inner;
	mesher_create(mesher);
	return result;
}
