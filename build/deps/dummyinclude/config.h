#ifndef ODE_CONFIG_H
#define ODE_CONFIG_H

/* Try to identify the platform */
#if defined(_XENON)
#define ODE_PLATFORM_XBOX360
#elif defined(SN_TARGET_PSP_HW)
#define ODE_PLATFORM_PSP
#elif defined(SN_TARGET_PS3)
#define ODE_PLATFORM_PS3
#elif defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)
#define ODE_PLATFORM_WINDOWS
#elif defined(__linux__)
#define ODE_PLATFORM_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
#define ODE_PLATFORM_OSX
#elif defined(__FreeBSD__)
#define ODE_PLATFORM_FREEBSD
#else
#error "Need some help identifying the platform!"
#endif

/* Additional platform defines used in the code */
#if defined(ODE_PLATFORM_WINDOWS) && !defined(WIN32)
#define WIN32
#endif

#if defined(__CYGWIN__) || defined(__MINGW32__)
#define CYGWIN
#endif

#if defined(ODE_PLATFORM_OSX)
#define macintosh
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "typedefs.h"

#endif // ODE_CONFIG_H
