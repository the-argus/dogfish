#pragma once
#include "constants/general.h"
#include <assert.h>
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>

/// This number squared is how many chunks will be stored in memory and rendered
/// at once.
#define RENDER_DISTANCE 4
#define RENDER_DISTANCE_HALF 2
static_assert((RENDER_DISTANCE % 2) == 0, "Render distance not divisible by 2");
static_assert(
	RENDER_DISTANCE == 2 * RENDER_DISTANCE_HALF,
	"RENDER_DISTANCE and RENDER_DISTANCE_HALF are not correctly related.");
/// The width and length of chunks, in voxels.
#define CHUNK_SIZE 16
/// Height of chunks, in voxels.
#define WORLD_HEIGHT 256

/// The number type for basic block info needed for rendering.
/// 0 = empty.
typedef uint8_t block_t;

/// Number type used to index into IntermediateVoxelData.voxels (a chunk of
/// voxels
typedef uint16_t voxel_index_t;
/// Same thing but signed, used for offsets
typedef int16_t voxel_index_signed_t;

/// Number type used to describe the coordinates of the current chunk
typedef int16_t chunk_index_t;

typedef struct
{
	chunk_index_t x;
	chunk_index_t z;
} ChunkCoords;

typedef struct
{
	ChunkCoords position;
	Mesh mesh;
} Chunk;

/// In-memory mesh data for all terrain surrounding every player.
/// Involves a lot of pointer indirection. Each mesh's vertices, indices, and
/// texcoords are allocated on separate buffers.
/// Always allocated to full capacity at game load time. "capacty" is only
/// runtime because we may want to changet he number of players per game in the
/// future?
typedef struct
{
	// amount allocated
	size_t capacity;
	// amount used (memory after this may be uninitialized)
	size_t count;
	Chunk chunks[0];
} TerrainData;

/// A buffer of data determining the contents of a chunk. Used to store the
/// generated state of the chunk before converting it to a mesh.
typedef struct
{
	ChunkCoords coords;
	block_t voxels[CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT];
	/// UV Rect for the texture of a given block_t, on a texture sampler stored
	/// somewhere else
	const Rectangle* uv_rect_lookup;
	size_t uv_rect_lookup_capacity;
} IntermediateVoxelData;

typedef struct
{
	voxel_index_t x;
	voxel_index_t y;
	voxel_index_t z;
} VoxelCoords;

typedef enum : uint8_t
{
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
	AXIS_MAX,
} Axis;

/// A description of a one-block difference from another block
typedef struct
{
	Axis axis;
	bool negative;
} VoxelOffset;

typedef struct
{
	Vector3 offset;
	Vector2 uv;
} VertexInfo;

typedef struct
{
#define VOXEL_FACE_INFO_NUM_VERTICES 6
	VertexInfo vertex_infos[VOXEL_FACE_INFO_NUM_VERTICES];
	Vector3 normal;
} VoxelFaceInfo;

float perlin_3d(float x, float y, float z);
float perlin_2d(float x, float y);

void init_noise();

/// Add a voxel offset to a voxel coordinate. If the offset is large, negative,
/// and causes integer overflow, then an assert will be thrown *in debug mode
/// only*.
VoxelCoords terrain_add_offset_to_voxel_coord(VoxelCoords coords,
											  VoxelOffset offset);
