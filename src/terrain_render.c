#include "terrain_render.h"
#include "threadutils.h"
#include <assert.h>
#include <external/glad.h>
#include <rlgl.h>
#include <stdlib.h>

#define MAX_MESH_VERTEX_BUFFERS 7 // Maximum vertex buffers (VBO) per mesh

unsigned int rlLoadVertexBufferCustom(const void* buffer, int size,
									  bool dynamic)
{
	unsigned int id = 0;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
	glGenBuffers(1, &id);
	glBindBuffer(GL_ARRAY_BUFFER, id);
	glBufferData(GL_ARRAY_BUFFER, size, buffer,
				 dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
#endif

	return id;
}
// Upload vertex data into a VAO (if supported) and VBO
void UploadTerrainMesh(Mesh* mesh, bool dynamic)
{
	if (mesh->vaoId > 0) {
		// Check if mesh has already been loaded in GPU
		TRACELOG(LOG_WARNING,
				 "VAO: [ID %i] Trying to re-load an already loaded mesh",
				 mesh->vaoId);
		return;
	}

	mesh->vboId =
		(unsigned int*)RL_CALLOC(MAX_MESH_VERTEX_BUFFERS, sizeof(unsigned int));
	mesh->vaoId = 0; // Vertex Array Object

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
	mesh->vaoId = rlLoadVertexArray();
	rlEnableVertexArray(mesh->vaoId);

	// NOTE: Vertex attributes must be uploaded considering default locations
	// points and available vertex data

	assert(mesh->normals != NULL);
	assert(mesh->colors == NULL);
	assert(mesh->tangents == NULL);
	assert(mesh->texcoords2 == NULL);
	assert(mesh->animVertices == NULL);

	// Enable vertex attributes: position (shader-location = 0)
	mesh->vboId[0] = rlLoadVertexBufferCustom(
		mesh->vertices, mesh->vertexCount * 3 * sizeof(float), dynamic);
	rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0);
	rlEnableVertexAttribute(0);

	// Enable vertex attributes: texcoords (shader-location = 1)
	mesh->vboId[1] = rlLoadVertexBufferCustom(
		mesh->texcoords, mesh->vertexCount * 2 * sizeof(float), dynamic);
	rlSetVertexAttribute(1, 2, RL_FLOAT, 0, 0, 0);
	rlEnableVertexAttribute(1);

	// WARNING: When setting default vertex attribute values, the values for
	// each generic vertex attribute is part of current state, and it is
	// maintained even if a different program object is used

	{
		// Enable vertex attributes: normals (shader-location = 2)
		void* normals =
			mesh->animNormals != NULL ? mesh->animNormals : mesh->normals;
		mesh->vboId[2] = rlLoadVertexBufferCustom(
			normals, mesh->vertexCount * 3 * sizeof(float), dynamic);
		rlSetVertexAttribute(2, 3, RL_FLOAT, 0, 0, 0);
		rlEnableVertexAttribute(2);
	}

	{
		// Default vertex attribute: color
		// WARNING: Default value provided to shader if location available
		float value[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // WHITE
		rlSetVertexAttributeDefault(3, value, SHADER_ATTRIB_VEC4, 4);
		rlDisableVertexAttribute(3);
	}

	{
		// Default vertex attribute: tangent
		// WARNING: Default value provided to shader if location available
		float value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		rlSetVertexAttributeDefault(4, value, SHADER_ATTRIB_VEC4, 4);
		rlDisableVertexAttribute(4);
	}

	{
		// Default vertex attribute: texcoord2
		// WARNING: Default value provided to shader if location available
		float value[2] = {0.0f, 0.0f};
		rlSetVertexAttributeDefault(5, value, SHADER_ATTRIB_VEC2, 2);
		rlDisableVertexAttribute(5);
	}

	if (mesh->indices != NULL) {
		mesh->vboId[6] = rlLoadVertexBufferElement(
			mesh->indices, mesh->triangleCount * 3 * sizeof(unsigned short),
			dynamic);
	}

	if (mesh->vaoId <= 0) {
		TRACELOG(LOG_ERROR, "Failed to upload mesh");
		threadutils_exit(EXIT_FAILURE);
	}

	rlDisableVertexArray();
#endif
}
