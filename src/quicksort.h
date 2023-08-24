#pragma once
#include <stdint.h>

/// Sort some arbitrary, uniformly-spaced array of floats
/// @param target: first float in array
/// @param nmemb: number of floats in the array
/// @param stride: distance in bytes between each float (sizeof(float) for
/// contiguous array)
void quicksort_inplace_float(float* target, uint16_t nmemb, uint8_t stride);

/// Sort some arbitrary, uniformly-spaced array of uint16_ts
/// @param target: first uint16 in array
/// @param nmemb: number of uint16 in the array
/// @param stride: distance in bytes between each uint16 (sizeof(uint16) for
/// contiguous array)
void quicksort_inplace_uint16(uint16_t* target, uint16_t nmemb, uint8_t stride);
