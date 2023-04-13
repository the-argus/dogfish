#include "debug.h"
#include "raylib.h"
#include "raymath.h"

static Vector3 debug_camera_position = {0};

void UseDebugCameraController(Camera *camera_to_move)
{
	Vector3 camera_forward =
		Vector3Subtract(camera_to_move->target, camera_to_move->position);
	camera_forward = Vector3Normalize(camera_forward);

	Vector3 camera_right =
		Vector3CrossProduct((Vector3){0, 1, 0}, camera_forward);

	int horizontal_input = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
	int vertical_input = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);

	Vector3 delta = Vector3Scale(camera_right, horizontal_input);
	delta = Vector3Add(delta, Vector3Scale(camera_forward, vertical_input));

	// prevent divide by 0 error from normalize
	if (Vector3Length(delta) == 0) {
		return;
	}

	delta = Vector3Normalize(delta);

	debug_camera_position = Vector3Add(debug_camera_position, delta);
	camera_to_move->position = debug_camera_position;
}

void Vector3Print(Vector3 vector, const char *name)
{
	char vec[80];
	Vector3ToString(vec, 80, vector);
	printf("%s: \t%s\n", name, vec);
}

void Vector3ToString(char *buffer, uint size, Vector3 vector)
{
	const char xprefix[] = "X: ";
	const char yprefix[] = "\tY: ";
	const char zprefix[] = "\tZ: ";
	const char num_format[] = "%f4.2";
	// number of characters taken up by a float formatted by num_format
	const int num_length_max = 12;
#ifndef RELEASE
	uint size_min = size < (num_length_max * 3) + strlen(xprefix) +
							   strlen(yprefix) + strlen(zprefix);
	if (size < size_min) {
		printf("Buffer size for vector3 string too small, must be at least "
			   "%d\n",
			   size_min);
		exit(EXIT_FAILURE);
	}
#endif

	uint i = 0;
	UNUSED(i);

	sprintf(buffer, xprefix);
	i = strlen(buffer);

	sprintf(buffer + i, num_format, vector.x);
	i = strlen(buffer);

	sprintf(buffer + i, yprefix);
	i = strlen(buffer);

	sprintf(buffer + i, num_format, vector.y);
	i = strlen(buffer);

	sprintf(buffer + i, zprefix);
	i = strlen(buffer);

	sprintf(buffer + i, num_format, vector.z);
	i = strlen(buffer);

#ifndef RELEASE
	assert(i <= size);
#endif
}
