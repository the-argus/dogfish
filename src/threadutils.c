#include "threadutils.h"
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t exit_mutex;
static pthread_mutexattr_t exit_mutex_attr;

void threadutils_init()
{
	pthread_mutexattr_init(&exit_mutex_attr);
	pthread_mutex_init(&exit_mutex, &exit_mutex_attr);
}

void threadutils_exit(int status)
{
	pthread_mutex_lock(&exit_mutex);

	// thread safe now wow!!!
	exit(status); // NOLINT

	// lol
	pthread_mutex_unlock(&exit_mutex);
}
