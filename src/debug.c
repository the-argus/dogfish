#include "debug.h"
#include "threadutils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Vector3 debug_camera_position = {0};

void UseDebugCameraController(Camera *camera_to_move)
{
	Vector3 camera_forward =
		Vector3Subtract(camera_to_move->target, camera_to_move->position);
	camera_forward = Vector3Normalize(camera_forward);

	Vector3 camera_right =
		Vector3CrossProduct(camera_to_move->up, camera_forward);

	int horizontal_input = IsKeyDown(KEY_A) - IsKeyDown(KEY_D);
	int vertical_input = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);

	Vector3 delta = Vector3Scale(camera_right, horizontal_input);
	delta = Vector3Add(delta, Vector3Scale(camera_forward, vertical_input));

	// prevent divide by 0 error from normalize
	if (Vector3Length(delta) != 0) {
		delta = Vector3Normalize(delta);
		delta = Vector3Scale(delta, 0.3f);

		debug_camera_position = Vector3Add(debug_camera_position, delta);
		camera_to_move->target = Vector3Add(camera_to_move->target, delta);
	}

	camera_to_move->position = debug_camera_position;
}

void Vector3Print(Vector3 vector, const char *name)
{
#define VECMAXBUF 80
	char vec[VECMAXBUF];
	Vector3ToString(vec, VECMAXBUF, vector);
	printf("%s: \t%s\n", name, vec);
#undef VECMAXBUF
}

void Vector3ToString(char *buffer, uint32_t size, Vector3 vector)
{
	const char xprefix[] = "X: ";
	const char yprefix[] = "\tY: ";
	const char zprefix[] = "\tZ: ";
	const char num_format[] = "%f4.2";
	// number of characters taken up by a float formatted by num_format
	const int num_length_max = 12;
#ifndef RELEASE
	uint32_t size_min = size < ((long)num_length_max * 3) + strlen(xprefix) +
								   strlen(yprefix) + strlen(zprefix);
	if (size < size_min) {
		// NOLINTNEXTLINE
		fprintf(stderr,
				"Buffer size for vector3 string too small, must be at least "
				"%d\n",
				size_min);
		threadutils_exit(EXIT_FAILURE);
	}
#endif

	size_t bytes = snprintf(buffer, size, "%s%f4.2%s%f4.2%s%f4.2", xprefix,
							vector.x, yprefix, vector.y, zprefix, vector.z);

	if (bytes < 0) {
		TraceLog(LOG_ERROR, "snprintf encoding err");
		return;
	}

	if (bytes >= size) {
		TraceLog(LOG_WARNING, "snprintf unable to write all characters of "
							  "vector to buffer. developer error");
	}
}
