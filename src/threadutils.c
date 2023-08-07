#include "threadutils.h"
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t exit_mutex;

void threadutils_init() { pthread_mutex_init(&exit_mutex, NULL); }

void threadutils_exit(int status)
{
	pthread_mutex_lock(&exit_mutex);

	// thread safe now wow!!!
	exit(status); // NOLINT

	// lol
	pthread_mutex_unlock(&exit_mutex);
}
