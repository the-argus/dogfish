#pragma once

/// Initialize mutexes and mutext attrs
void threadutils_init();

/// Thread-safe exit
void threadutils_exit(int status);

#define CHECKMEM(varname)                                                \
	if (#varname == NULL) {                                              \
		TraceLog(LOG_FATAL, "Memory allocation failed in %s at line %d", \
				 __FUNCTION__, __LINE__);                                \
		threadutils_exit(EXIT_FAILURE);                                  \
	}
