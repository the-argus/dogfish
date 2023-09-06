#pragma once
#include "constants/general.h"
#include "mesher.h"
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
	/// Number of players who have this chunk in their render distance
	uint8_t loaders;
	ChunkCoords position;
	Mesh mesh;
} Chunk;

typedef struct
{
	size_t capacity;
	size_t count;
	size_t indices[0];
} AvailableIndicesStack;

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
	AvailableIndicesStack* available_indices;
	Chunk chunks[0];
} TerrainData;

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

enum Sides : uint8_t
{
	SOUTH = 0,
	NORTH,
	WEST,
	EAST,
	UP,
	DOWN,
	NUM_SIDES
};

/// Typedef used to record a single bit per face of a voxel. Useful for storing
/// information about what sides of a voxel are visible.
typedef struct
{
	uint8_t south : 1;
	uint8_t north : 1;
	uint8_t west : 1;
	uint8_t east : 1;
	uint8_t up : 1;
	uint8_t down : 1;
} VoxelFaces;

typedef struct
{
	size_t size;
	size_t indices[RENDER_DISTANCE * RENDER_DISTANCE * NUM_PLANES];
} UnneededChunkList;

// get the block_t for a single voxel given its coordinates
block_t terrain_generate_voxel(ChunkCoords chunk, VoxelCoords voxel);
void terrain_add_voxel_to_mesher(Mesher* restrict mesher, VoxelCoords coords,
								 ChunkCoords chunk_coords, VoxelFaces faces,
								 const Rectangle* restrict uv_rect_lookup,
								 block_t voxel);

void terrain_mesher_add_face(Mesher* restrict mesher,
							 const Vector3* restrict position,
							 const VoxelFaceInfo* restrict face);

/// Inserts a new chunk into the terrain data
/// returns the index at which the item was inserted
size_t terrain_mesh_insert(TerrainData* restrict data, const Chunk* new_chunk);

/// Add some indices to the internal queue of available chunk indices
void terrain_data_mark_indices_free(TerrainData* restrict data,
									const UnneededChunkList* restrict unneeded);

/// Remove all chunks with no loaders, making the buffer of chunks contiguous
/// again.
void terrain_data_normalize(TerrainData* data);

bool terrain_voxel_is_solid(block_t type);

float perlin_3d(float x, float y, float z);
float perlin_2d(float x, float y);

void init_noise();

/// Add a voxel offset to a voxel coordinate. If the offset is large, negative,
/// and causes integer overflow, then an assert will be thrown *in debug mode
/// only*.
VoxelCoords terrain_add_offset_to_voxel_coord(VoxelCoords coords,
											  VoxelOffset offset);
