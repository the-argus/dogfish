#include "airplane.h"
#include "bullet.h"
#include "gamestate.h"
#include "input.h"
#include "physics.h"
#include "shorthand.h"
#include "threadutils.h"
#include <raymath.h>
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define AIRPLANE_DEBUG_WIDTH 0.5
#define AIRPLANE_DEBUG_LENGTH 2
#define AIRPLANE_DEBUG_COLOR RED
#define AIRPLANE_MOVE_SPEED 0.1f
#define AIRPLANE_BOOST_SPEED 0.15f
#define AIRPLANE_MASS 5.4
#define INITIAL_AIRPLANE_POS_P1 5, 10, 0
#define INITIAL_AIRPLANE_POS_P2 -5, 10, 0

#define CAMERA_FIRST_PERSON_MIN_CLAMP 89.0f
#define CAMERA_FIRST_PERSON_MAX_CLAMP -89.0f
// radians per frame
#define CAMERA_MOUSE_MOVE_SENSITIVITY 0.5f
#define CAMERA_DISTANCE 5

// rendering stuff
#define MAX_LIGHTS 4

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1

typedef struct
{
	AABB aabb;
	Vector3 position;
	float speed;
	Quaternion direction;
} Airplane;

#define NUM_PLANES 2
static Airplane planes[NUM_PLANES];
#define AIRPLANE_MODEL_SCALEFACTOR 0.1f
static const char* model_filename = "assets/models/airplane.gltf";
static const char* diffuse_texture_filenames[] = {
	"assets/textures/airplane/hainan_diffuse.png",
	"assets/textures/airplane/lufthansa_diffuse.png",
};
static const char* metallic_texture_filename =
	"assets/textures/airplane/metallic.png";
static const char* normal_texture_filename =
	"assets/textures/airplane/normal.png";
// TODO: normals and stuff in here is the reason the plane model looks weird
static const char* airplane_frag_shader_filename =
	"assets/materials/airplane.frag";
static Shader shader;
static Material materials[NUM_PLANES];
static Model models[NUM_PLANES];
static Texture2D diffuse[NUM_PLANES];
static Texture2D metallic;
static Texture2D normal;
static AABBBatchOptions airplane_data_aabb_options;
static Vector3BatchOptions airplane_data_position_options;
static QuaternionBatchOptions airplane_data_direction_options;
static FloatBatchOptions airplane_data_speed_options;
static Light lights[MAX_LIGHTS] = {0};
static const Vector4 ambientLight = {0.0f, 0.0f, 0.0f, 1.0f};
static Matrix airplane_default_transform;

static void airplane_update_velocity(Airplane* restrict plane,
									 const Keystate* restrict keys,
									 const ControllerState* restrict controls);
static inline void airplane_update_p1(float delta_time);
static inline void airplane_update_p2(float delta_time);

void airplane_init()
{
	metallic = LoadTexture(metallic_texture_filename);
	normal = LoadTexture(normal_texture_filename);
	shader = LoadShader(0, airplane_frag_shader_filename);

	shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
	// insert ambient light as uniform
	ShaderLocationIndex ambientLoc = GetShaderLocation(shader, "ambient");
	SetShaderValue(shader, ambientLoc, (float*)&ambientLight,
				   SHADER_UNIFORM_VEC4);

	// all lights are empty except one directional light
	lights[0] = CreateLight(LIGHT_DIRECTIONAL, (Vector3){100.0f, 100.0f, 0},
							Vector3Zero(), WHITE, shader);

	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		planes[i] = (Airplane){
			.aabb =
				(AABB){
					.x = AIRPLANE_DEBUG_WIDTH,
					.y = AIRPLANE_DEBUG_WIDTH,
					.z = AIRPLANE_DEBUG_LENGTH,
				},
			.position = (Vector3){0.0f, 0.0f, 0.0f},
			.direction = QuaternionFromEuler(0, 90 * DEG2RAD, 0),
			.speed = 0.1f,
		};

		models[i] = LoadModel(model_filename);
		diffuse[i] = LoadTexture(diffuse_texture_filenames[i]);
		// TODO: it's possible for the gltf file to have relative texture paths
		// embedded in it, so we could have the normal and specular specified
		// there. raylib even loads them by default with LoadModel
		// diffuse
		SetMaterialTexture(&models[i].materials[0], MATERIAL_MAP_ALBEDO,
						   diffuse[i]);
		// specular
		SetMaterialTexture(&models[i].materials[0], MATERIAL_MAP_METALNESS,
						   metallic);
		SetMaterialTexture(&models[i].materials[0], MATERIAL_MAP_NORMAL,
						   normal);

		// the airplane model has two materials
		models[i].materials[0].shader = shader;
		models[i].materials[1].shader = shader;
	}

	planes[0].position = (Vector3){INITIAL_AIRPLANE_POS_P1};
	planes[1].position = (Vector3){INITIAL_AIRPLANE_POS_P2};

	// rotate the model to be pointing along positive X
	airplane_default_transform = MatrixRotateXYZ((Vector3){0, 90 * DEG2RAD, 0});

	airplane_data_aabb_options = (AABBBatchOptions){
		.count = NUM_PLANES,
		.first = &planes[0].aabb,
		.stride = sizeof(Airplane),
	};

	airplane_data_position_options = (Vector3BatchOptions){
		.count = NUM_PLANES,
		.first = &planes[0].position,
		.stride = sizeof(Airplane),
	};

	airplane_data_direction_options = (QuaternionBatchOptions){
		.count = NUM_PLANES,
		.first = &planes[0].direction,
		.stride = sizeof(Airplane),
	};

	airplane_data_speed_options = (FloatBatchOptions){
		.count = NUM_PLANES,
		.first = &planes[0].speed,
		.stride = sizeof(Airplane),
	};

	FullCamera* cameras = gamestate_get_cameras_mutable();
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		cameras[i].camera.target = planes[i].position;
	}
	cameras = NULL;
	gamestate_return_cameras_mutable();
}

void airplane_cleanup()
{
	UnloadTexture(normal);
	UnloadTexture(metallic);
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		UnloadModel(models[i]);
		UnloadTexture(diffuse[i]);
	}
}

static CollisionHandlerReturnCode
airplane_on_collision(uint16_t bullet_handle, uint16_t airplane_handle,
					  Contact* contact)
{
	UNUSED(contact);
	uint8_t player_who_fired =
		bullet_get_source((BulletHandle){.raw = bullet_handle});
	TraceLog(LOG_INFO, "Player %d hit by bullet from player %d",
			 airplane_handle + 1, player_who_fired);
#ifndef NDEBUG
	if (airplane_handle + 1 == player_who_fired) {
		TraceLog(LOG_WARNING, "Player %d shot themselves.", player_who_fired);
	}
#endif
	return CONTINUE;
}

void airplane_update(float delta_time)
{
	const Inputstate* input = gamestate_get_inputstate();
	const FullCamera* cameras = gamestate_get_cameras();
	static_assert((sizeof(input->cursor) / sizeof(input->cursor[0])) ==
					  NUM_PLANES,
				  "Different number of planes and sets of controls.");
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		// all planes shoot in the same way
		if (input->cursor[i].shoot || input->controller[i].shoot) {
			// bullet shoots in the direction you're moving...
			BulletCreateOptions bullet_options = {
				.bullet =
					(Bullet){
						.direction = QuaternionFromEuler(
							0, cameras[i].angle.x + PI, cameras[i].angle.y),
						.position = planes[i].position,
					},
				.source = (Source)i,
			};
			bullet_create(&bullet_options);
		}

		// also update the plane's velocity
		airplane_update_velocity(&planes[i], &input->keys[i],
								 &input->controller[i]);

		// player-specific changes
		switch (i) {
		case 0:
			airplane_update_p1(delta_time);
			break;
		case 1:
			airplane_update_p2(delta_time);
			break;
		default:
			TraceLog(LOG_ERROR, "huh? %d", i);
			threadutils_exit(EXIT_FAILURE);
			break;
		}
	}

	bullet_move_and_collide_with(
		&airplane_data_aabb_options, &airplane_data_position_options,
		&airplane_data_direction_options, &airplane_data_speed_options,
		airplane_on_collision);
}

void airplane_draw()
{
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		// model rotates toward where it is moving
		models[i].transform = MatrixMultiply(
			MatrixMultiply(airplane_default_transform,
						   QuaternionToMatrix(planes[i].direction)),
			MatrixScale(AIRPLANE_MODEL_SCALEFACTOR, AIRPLANE_MODEL_SCALEFACTOR,
						AIRPLANE_MODEL_SCALEFACTOR));
		DrawModel(models[i], planes[i].position, 1.0f, WHITE);
	}
}

static inline void airplane_update_p1(float delta_time)
{
	// get the first camera
	FullCamera* camera = &gamestate_get_cameras_mutable()[0];
#ifdef DEBUG_CAMERA
	UseDebugCameraController(camera[0]);
#else
	// set the camera to be at the location of the plane
	Vector2 cursor_delta = total_cursor(0);

	// look at the plane
	camera->camera.target = planes[0].position;
	// this will change later
	camera->camera.position = planes[0].position;

	// rotate by angles x and y
	camera->angle = Vector2Add(
		camera->angle,
		Vector2Scale(cursor_delta, CAMERA_MOUSE_MOVE_SENSITIVITY * delta_time));

	// clamp y
	if (camera->angle.y > CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD) {
		camera->angle.y = CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD;
	} else if (camera->angle.y < CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD) {
		camera->angle.y = CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD;
	}

	// clamp x
	camera->angle.x -= ((int)(camera->angle.x / (2 * PI))) * (2 * PI);

	// make the camera's position around the player respect angles
	Vector3 move_by = {CAMERA_DISTANCE, 0, 0};
	move_by =
		Vector3RotateByAxisAngle(move_by, (Vector3){0, 1, 0}, camera->angle.x);

	// right vector is the axis
	Vector3 axis = Vector3CrossProduct((Vector3){0, 1, 0}, move_by);
	move_by = Vector3RotateByAxisAngle(move_by, axis, camera->angle.y);
	camera->camera.position = Vector3Add(camera->camera.position, move_by);
#endif
	gamestate_return_cameras_mutable();
}

static inline void airplane_update_p2(float delta_time) {}

static void airplane_update_velocity(Airplane* restrict plane,
									 const Keystate* restrict keys,
									 const ControllerState* restrict controls)
{
	const bool thrust = controls->boost || keys->boost;
	plane->speed = thrust ? AIRPLANE_BOOST_SPEED : AIRPLANE_MOVE_SPEED;
}
