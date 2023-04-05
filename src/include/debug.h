#include "raylib.h"
#include "shorthand.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
