#pragma once

/// Initialize mutexes and mutext attrs
void threadutils_init();

/// Thread-safe exit
void threadutils_exit(int status);
