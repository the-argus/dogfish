#include "terrain_internal.h"
#include "quicksort.h"
#include "threadutils.h"
#include <FastNoiseLite.h>
#include <raymath.h>

#define MAX_INTERP_POINTS 6
// a struct defining basically a piecewise function on which to transform
// noise after sampling.
typedef struct
{
	uint8_t point_count;
	Vector2 interp_points[MAX_INTERP_POINTS];
} InterpPoints;
// some noise generation information and transformation information in a pair.
typedef struct
{
	fnl_state settings;
	InterpPoints transform;
	float transform_scale;
} NoiseLayer;

static fnl_state terrain_noise_perlin_main;

// continentialness
static NoiseLayer terrain_noise_height_base;
// peaks and valleys
static NoiseLayer terrain_noise_height_detail;
// the height at which the 3D perlin noise generated value density is unchanged.
// below, it will be more negative, and above it will be more positive
static const float terrain_height = (float)WORLD_HEIGHT / 2;

static const VoxelCoords max_voxelcoord = {
	.x = CHUNK_SIZE,
	.y = WORLD_HEIGHT,
	.z = CHUNK_SIZE,
};

static void terrain_interp_add_point(InterpPoints* points, Vector2 point);
/// Accepts some interpolation points, as well as a noise value between -1 and 1
/// and maps that by the function described by the interpolation points.
/// Returns a value between -1 and 1.
static float terrain_interp_transform(const InterpPoints* points, float input);
static void terrain_interp_create(InterpPoints* points, float left,
								  float right);
static float perlin_3d(const fnl_state* noise, float x, float y, float z);
static float perlin_2d(const fnl_state* noise, float x, float y);

void init_noise()
{
	terrain_noise_perlin_main = fnlCreateState();
	terrain_noise_perlin_main.octaves = 8;
	terrain_noise_perlin_main.noise_type = FNL_NOISE_PERLIN;
	terrain_noise_perlin_main.lacunarity = 2;

	// set up main height layer
	terrain_noise_height_base = (NoiseLayer){
		.settings = fnlCreateState(),
		.transform = {0},
		.transform_scale = 30,
	};
	terrain_interp_create(&terrain_noise_height_base.transform, 0.0f, 1.0f);
	terrain_interp_add_point(&terrain_noise_height_base.transform,
							 (Vector2){0.3f, 0.1f});
	terrain_interp_add_point(&terrain_noise_height_base.transform,
							 (Vector2){0.4f, 0.2f});
	terrain_interp_add_point(&terrain_noise_height_base.transform,
							 (Vector2){0.5f, 0.8f});
	terrain_noise_height_base.settings.octaves = 8;
	terrain_noise_height_base.settings.noise_type = FNL_NOISE_PERLIN;
	terrain_noise_height_base.settings.lacunarity = 2;

	// also do the details layer
	terrain_noise_height_detail = (NoiseLayer){
		.settings = fnlCreateState(),
		.transform = {0},
		.transform_scale = 10,
	};
	terrain_interp_create(&terrain_noise_height_detail.transform, 0.1f, 1);
	terrain_interp_add_point(&terrain_noise_height_detail.transform,
							 (Vector2){0.5f, 0.1f});
	// add a peak right in the middle
	// terrain_interp_add_point(&terrain_noise_height_detail.transform,
	// 						 (Vector2){.x = 0.5f, .y = 1.0f});
	terrain_noise_height_detail.settings.octaves = 2;
	terrain_noise_height_detail.settings.noise_type = FNL_NOISE_PERLIN;
	terrain_noise_height_detail.settings.lacunarity = 1;
}

block_t terrain_generate_voxel(ChunkCoords chunk, VoxelCoords voxel)
{
	float x = (float)voxel.x + (float)(chunk.x * max_voxelcoord.x);
	float y = (float)voxel.y;
	float z = (float)voxel.z + (float)(chunk.z * max_voxelcoord.z);

	// both values between -1 and 1
	const float base_height_generated =
		perlin_2d(&terrain_noise_height_base.settings, x, z);

	const float base_height =
		terrain_interp_transform(&terrain_noise_height_base.transform,
								 Clamp(base_height_generated, -1, 1)) *
		terrain_noise_height_base.transform_scale;

	const float detail_height_generated =
		perlin_2d(&terrain_noise_height_detail.settings, x, z);
	const float detail_height =
		terrain_interp_transform(&terrain_noise_height_detail.transform,
								 Clamp(detail_height_generated, -1, 1)) *
		terrain_noise_height_detail.transform_scale;

	const float height = Clamp(terrain_height + base_height + detail_height, 0,
							   WORLD_HEIGHT - 1);

	// value between -1 and 1
	const float density = perlin_3d(&terrain_noise_perlin_main, x, y, z);
	const float final = Clamp(((y - height) / WORLD_HEIGHT) + density, -1, 1);
	// TraceLog(LOG_INFO,
	// 		 "BASE HEIGHT: %f\nDETAIL HEIGHT: %f\nHEIGHT: %f\nDENSITY: "
	// 		 "%f\nFINAL: %f",
	// 		 base_height_generated, detail_height, height, density, final);
	// TraceLog(LOG_INFO, "BASE HEIGHT FOR %f: %f", x, detail_height_generated);

	if (final < 0) {
		return 1;
	}
	return 0;
}

static float perlin_3d(const fnl_state* noise, float x, float y, float z)
{
	float gen = fnlGetNoise3D(noise, x, y, z);
	return gen;
}

static float perlin_2d(const fnl_state* noise, float x, float y)
{
	float gen = fnlGetNoise2D(noise, x, y);
	return gen;
}

VoxelCoords terrain_add_offset_to_voxel_coord(VoxelCoords coords,
											  const VoxelOffset offset)
{
	switch (offset.axis) {
	case AXIS_X:
		coords.x += offset.negative ? -1 : 1;
		break;
	case AXIS_Y:
		coords.y += offset.negative ? -1 : 1;
		break;
	case AXIS_Z:
		coords.z += offset.negative ? -1 : 1;
		break;
	default:
		TraceLog(LOG_WARNING, "invalid voxel offset axis");
#ifndef NDEBUG
		threadutils_exit(EXIT_FAILURE);
#endif
	}

	assert(coords.x >= 0 && coords.x < max_voxelcoord.x);
	assert(coords.y >= 0 && coords.y < max_voxelcoord.y);
	assert(coords.y >= 0 && coords.z < max_voxelcoord.z);

	return coords;
}

size_t terrain_mesh_insert(TerrainData* restrict data, const Chunk* new_chunk)
{
	if (!(data->count <= data->capacity)) {
		TraceLog(LOG_INFO, "data count %d and available indices %d",
				 data->count, data->available_indices->count);
	}
	assert(data->count <= data->capacity);
	size_t index = data->count;

	// only add to the end if there are no available spots
	if (data->available_indices->count == 0) {
		// preemptively add to the count of data
		++data->count;
	} else {
		index = data->available_indices
					->indices[data->available_indices->count - 1];
		--data->available_indices->count;
	}
	data->chunks[index] = *new_chunk;
	return index;
}

void terrain_data_mark_indices_free(TerrainData* restrict data,
									const UnneededChunkList* restrict unneeded)
{
	// append all of the unneededs to available indices
	for (size_t i = 0; i < unneeded->size; ++i) {
		data->available_indices->indices[data->available_indices->count] =
			unneeded->indices[i];
		++data->available_indices->count;
		assert(data->available_indices->count <=
			   data->available_indices->capacity);
	}
}

void terrain_data_normalize(TerrainData* data)
{
	if (data->available_indices->count == 0) {
		return;
	}

	// remove in order
	quicksort_inplace_size_t(&data->available_indices->indices[0],
							 data->available_indices->count, sizeof(size_t));

	// the largest available index should not be greater than the number of
	// chunks
	assert(
		data->available_indices->indices[data->available_indices->count - 1] <
		data->count);
	assert(data->available_indices->count <= data->count);

#ifndef NDEBUG
	for (size_t i = 0; i < data->available_indices->count; ++i) {
		assert(data->chunks[data->available_indices->indices[i]].loaders == 0);
	}
#endif

	for (int i = (int)data->available_indices->count - 1; i >= 0; --i) {
		// starting with the largest available index
		size_t available = data->available_indices->indices[i];
		if (available == data->count - 1) {
			// the last item in the array of chunks needs to be removed
			--data->count;
			continue;
		}
		// otherwise, we can move this one
		data->chunks[available] = data->chunks[data->count - 1];
		--data->count;
	}

	// all indices should be removed
	data->available_indices->count = 0;
}

bool terrain_voxel_is_solid(block_t type) { return type != 0; }

void terrain_mesher_add_face(Mesher* restrict mesher,
							 const Vector3* restrict position,
							 const VoxelFaceInfo* restrict face)
{
	mesher->normal = face->normal;
	for (uint8_t i = 0; i < NUM_SIDES; ++i) {
		mesher->uv = face->vertex_infos[i].uv;
		mesher_push_vertex(mesher, &face->vertex_infos[i].offset, position);
	}
}

void terrain_add_voxel_to_mesher(Mesher* restrict mesher, VoxelCoords coords,
								 ChunkCoords chunk_coords, VoxelFaces faces,
								 const Rectangle* restrict uv_rect_lookup,
								 block_t voxel)
{
	const Rectangle* uv_rect = &uv_rect_lookup[voxel];
	const Vector3 rl_coords = (Vector3){
		(float)((voxel_index_signed_t)coords.x +
				(chunk_coords.x * max_voxelcoord.x)),
		(float)coords.y,
		(float)((voxel_index_signed_t)coords.z +
				(chunk_coords.z * max_voxelcoord.z)),
	};

	// z-
	if (faces.north) {
		const VoxelFaceInfo north_face = {
			.normal = {0, 0, -1},
			.vertex_infos = {
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {1, 0, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &north_face);
	}

	// z+
	if (faces.south) {
		const VoxelFaceInfo south_face = {
			.normal = {0, 0, 1},
			.vertex_infos = {
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 1, 1}, .uv = {uv_rect->x, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &south_face);
	}

	// x+
	if (faces.west) {
		const VoxelFaceInfo west_face = {
			.normal = {1, 0, 0},
			.vertex_infos = {
				{.offset = {1, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &west_face);
	}

	// x-
	if (faces.east) {
		const VoxelFaceInfo east_face = {
			.normal = {-1, 0, 0},
			.vertex_infos = {
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {0, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &east_face);
	}

	if (faces.up) {
		const VoxelFaceInfo east_face = {
			.normal = {0, 1, 0},
			.vertex_infos = {
				{.offset = {0, 1, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {1, 1, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {0, 1, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {0, 1, 1}, .uv = {uv_rect->x, uv_rect->height}},
				{.offset = {1, 1, 1}, .uv = {uv_rect->width, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &east_face);
	}

	if (faces.down) {
		const VoxelFaceInfo east_face = {
			.normal = {0, -1, 0},
			.vertex_infos = {
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 0, 0}, .uv = {uv_rect->width, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 0, 0}, .uv = {uv_rect->x, uv_rect->y}},
				{.offset = {1, 0, 1}, .uv = {uv_rect->width, uv_rect->height}},
				{.offset = {0, 0, 1}, .uv = {uv_rect->x, uv_rect->height}},
			}};
		terrain_mesher_add_face(mesher, &rl_coords, &east_face);
	}
}

static float terrain_interp_transform(const InterpPoints* points, float input)
{
#ifndef NDEBUG
	// need two points at either end
	assert(points->point_count >= 2);
	assert(points->point_count < MAX_INTERP_POINTS);
	// assert that points are in lowest to highest order
	for (int16_t i = 0; i < points->point_count - 1; ++i) {
		assert(points->interp_points[i].x < points->interp_points[i + 1].x);
	}
	assert(points->interp_points[0].x == 0);
	assert(points->interp_points[points->point_count - 1].x == 1);
	// all interpolation points must have xs and ys between 0 and 1
	for (int16_t i = 0; i < points->point_count; ++i) {
		assert(points->interp_points[i].x >= 0 &&
			   points->interp_points[i].x <= 1);
		assert(points->interp_points[i].y >= 0 &&
			   points->interp_points[i].y <= 1);
	}
	assert(input >= -1 && input <= 1);
#endif

	// map [-1, 1] to [0, 1]
	float mapped_input = (input / 2) + 0.5f;
	assert(mapped_input >= 0 && mapped_input <= 1);

	Vector2 const* left = &points->interp_points[0];
	Vector2 const* right = &points->interp_points[points->point_count - 1];

	while (left->x < mapped_input && left < right) {
		left += 1;
	}
	left -= 1;

	while (right->x >= mapped_input && left < right) {
		right -= 1;
	}
	right += 1;
	assert(right != left);
	assert(right >= points->interp_points);
	assert(left >= points->interp_points);
	assert(right <= &points->interp_points[points->point_count - 1]);
	assert(left <= &points->interp_points[points->point_count - 1]);

	// we should now have the nearest left and right points
	const float diff_total = right->x - left->x;
	const float diff_input = right->x - left->x;
	assert(diff_total >= 0);
	assert(diff_input >= 0);
	assert(diff_input <= diff_total);

	const float lerp = Lerp(left->y, right->y, diff_input / diff_total);
	// re-map value to -1, 1 before returning
	const float unmapped = (lerp - 0.5f) * 2;
	assert(unmapped >= -1 && unmapped <= 1);

	return unmapped;
}

static void terrain_interp_create(InterpPoints* points, float left, float right)
{
	points->point_count = 0;
	terrain_interp_add_point(points, (Vector2){.x = 0, .y = left});
	terrain_interp_add_point(points, (Vector2){.x = 1, .y = right});
}

static void terrain_interp_add_point(InterpPoints* points, Vector2 point)
{
	assert(points->point_count + 1 < MAX_INTERP_POINTS);
	assert(point.x >= 0 && point.x <= 1);
	assert(point.y >= 0 && point.y <= 1);

	for (int16_t i = 0; i < points->point_count; ++i) {
		if (points->interp_points[i].x > point.x) {
			// item at current index and all others move to the right
			for (int16_t j = points->point_count; j > i; --j) {
				assert(j < MAX_INTERP_POINTS);
				assert(j > 0);
				points->interp_points[j] = points->interp_points[j - 1];
			}

			// should have been moved to the right
			assert(points->interp_points[i].x ==
				   points->interp_points[i + 1].x);
			assert(points->interp_points[i].y ==
				   points->interp_points[i + 1].y);
			// copy in new item
			points->interp_points[i] = point;
			++points->point_count;
			return;
		}
	}

	// if we didnt find a point which is bigger on x, just put this one at the
	// end
	points->interp_points[points->point_count] = point;
	++points->point_count;
}
