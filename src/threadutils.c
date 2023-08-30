#include "threadutils.h"
#ifndef _WIN32
#include <pthread.h>
#endif
#include <stdlib.h>

#ifndef _WIN32
static pthread_mutex_t exit_mutex;
#endif

void threadutils_init()
{
#ifndef _WIN32
	pthread_mutex_init(&exit_mutex, NULL);
#endif
}

void threadutils_exit(int status)
{
#ifndef _WIN32
	pthread_mutex_lock(&exit_mutex);
#endif

	// thread safe now wow!!!
	exit(status); // NOLINT

	// lol
#ifndef _WIN32
	pthread_mutex_unlock(&exit_mutex);
#endif
}
