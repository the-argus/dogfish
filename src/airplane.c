#include "airplane.h"
#include "bullet.h"
#include "gamestate.h"
#include "input.h"
#include "threadutils.h"
#include <raymath.h>

#define AIRPLANE_DEBUG_WIDTH 0.5
#define AIRPLANE_DEBUG_LENGTH 2
#define AIRPLANE_DEBUG_COLOR RED
#define AIRPLANE_MASS 5.4
#define INITIAL_AIRPLANE_POS_P1 5, 10, 0
#define INITIAL_AIRPLANE_POS_P2 -5, 10, 0

#define CAMERA_FIRST_PERSON_MIN_CLAMP -1.0f
#define CAMERA_FIRST_PERSON_MAX_CLAMP -179.0f
#define CAMERA_MOUSE_MOVE_SENSITIVITY 0.5f

typedef struct
{
	AABB aabb;
	Vector3 position;
	Quaternion velocity;
} Airplane;

#define NUM_PLANES 2
static Airplane planes[NUM_PLANES];
static Model models[NUM_PLANES];
static Color colors[NUM_PLANES] = {BLUE, GREEN};

AABBBatchOptions airplane_data_aabb_options;
Vector3BatchOptions airplane_data_position_options;
QuaternionBatchOptions airplane_data_velocity_options;

static void airplane_update_velocity(const Airplane* plane,
									 const Keystate* keys,
									 const ControllerState* controls);
static inline void airplane_update_p1(float delta_time);
static inline void airplane_update_p2(float delta_time);

void airplane_init()
{
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		planes[i] = (Airplane){
			.aabb =
				(AABB){
					.x = AIRPLANE_DEBUG_WIDTH,
					.y = AIRPLANE_DEBUG_WIDTH,
					.z = AIRPLANE_DEBUG_LENGTH,
				},
			.position = (Vector3){0.0f, 0.0f, 0.0f},
			.velocity = QuaternionIdentity(),
		};

		models[i] = LoadModelFromMesh(GenMeshCylinder(
			AIRPLANE_DEBUG_WIDTH / 2.0f, AIRPLANE_DEBUG_LENGTH, 10));
	}

	planes[0].position = (Vector3){INITIAL_AIRPLANE_POS_P1};
	planes[1].position = (Vector3){INITIAL_AIRPLANE_POS_P2};

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

	airplane_data_velocity_options = (QuaternionBatchOptions){
		.count = NUM_PLANES,
		.first = &planes[0].velocity,
		.stride = sizeof(Airplane),
	};
}

void airplane_cleanup()
{
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		UnloadModel(models[i]);
	}
}

static CollisionHandlerReturnCode
airplane_on_collision(uint16_t bullet_handle, uint16_t airplane_handle,
					  Contact* contact)
{
	TraceLog(LOG_INFO, "Player %d hit by bullet", airplane_handle + 1);
	return CONTINUE;
}

void airplane_update(float delta_time)
{
	const Inputstate* input = gamestate_get_inputstate();
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		// all planes shoot in the same way
		if (input->cursor[i].shoot || input->controller[i].shoot) {
			// bullet shoots in the direction you're moving...
			Quaternion bullet_velocity =
				QuaternionNormalize(planes[i].velocity);
			bullet_create(&planes[i].position, &bullet_velocity, i);
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
		&airplane_data_velocity_options, airplane_on_collision);
}

void airplane_draw()
{
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		// model rotates toward where it is moving
		models[i].transform =
			QuaternionToMatrix(QuaternionNormalize(planes[i].velocity));
		DrawModel(models[i], planes[i].position, 1.0, colors[i]);
	}
}

static inline void airplane_update_p1(float delta_time)
{
	FullCamera* camera = gamestate_get_cameras_mutable();
#ifdef DEBUG_CAMERA
	UseDebugCameraController(camera[0]);
#else
	// set the camera to be at the location of the plane
	Vector2 cursor_delta = total_cursor(0);
	camera->camera.target = planes[0].position; // look at the plane

	Vector3 camera_diff = {0, 5, 0};

	// rotate by angles x and y
	camera->angle =
		Vector2Scale(cursor_delta, CAMERA_MOUSE_MOVE_SENSITIVITY * delta_time);

	// clamp y
	if (camera->angle.y > CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD) {
		camera->angle.y = CAMERA_FIRST_PERSON_MIN_CLAMP * DEG2RAD;
	} else if (camera->angle.y < CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD) {
		camera->angle.y = CAMERA_FIRST_PERSON_MAX_CLAMP * DEG2RAD;
	}

	// clamp x
	camera->angle.x -= ((int)(camera->angle.x / (2 * PI))) * (2 * PI);

	// apply rotation to camera_diff
	Quaternion camera_rot =
		QuaternionFromEuler(camera->angle.y, camera->angle.x, 0);
	camera_diff = Vector3RotateByQuaternion(camera_diff, camera_rot);
	camera->camera.position = Vector3Add(camera->camera.position, camera_diff);
#endif
	gamestate_return_cameras_mutable();
}

static inline void airplane_update_p2(float delta_time) {}

static void airplane_update_velocity(const Airplane* plane,
									 const Keystate* keys,
									 const ControllerState* controls)
{
	// Vector2 input = total_input(plane - planes);
	// bool thrust = controls->boost || keys->boost;
	// Vector3 movement =
	// 	Vector3Scale(forward, thrust ? PLANE_BOOST_SPEED : PLANE_MOVE_SPEED);
}
