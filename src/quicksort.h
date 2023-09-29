#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint16_t quicksort_index_t;

/// A function which is called by the quicksort implementation whenever it
/// swaps the indices of two elements. These are usually not the final sorted
/// positions of the elements. It is called *after* the two elements have just
/// been swapped.
typedef void (*quicksort_swap_handler_t)(void* user_data,
										 quicksort_index_t left,
										 quicksort_index_t right);

/// Sort some arbitrary, uniformly-spaced array of floats
/// @param target: first float in array
/// @param nmemb: number of floats in the array
/// @param stride: distance in bytes between each float (sizeof(float) for
/// contiguous array)
void quicksort_inplace_float(float* target, quicksort_index_t nmemb,
							 uint8_t stride);

/// Sort some arbitrary, uniformly-spaced array of uint16_ts
/// @param target: first uint16 in array
/// @param nmemb: number of uint16 in the array
/// @param stride: distance in bytes between each uint16 (sizeof(uint16) for
/// contiguous array)
void quicksort_inplace_uint16(uint16_t* target, quicksort_index_t nmemb,
							  uint8_t stride);

/// Sort some arbitrary, uniformly-spaced array of size_ts
/// @param target: first size_t in array
/// @param nmemb: number of size_t in the array
/// @param stride: distance in bytes between each size_t (sizeof(size_t) for
/// contiguous array)
void quicksort_inplace_size_t(size_t* target, quicksort_index_t nmemb,
							  uint8_t stride);

/// Sort some arbitrary, uniformly-spaced array of floats
/// @param target: first float in array
/// @param nmemb: number of floats in the array
/// @param stride: distance in bytes between each float (sizeof(float) for
/// contiguous array)
/// @param handler: function that gets called whenever an element in the array
/// is moved.
/// @param user_data: pointer propagated to the handler
void quicksort_inplace_handler_float(float* target, quicksort_index_t nmemb,
									 uint8_t stride,
									 quicksort_swap_handler_t handler,
									 void* user_data);

/// Returns true if greater is indeed greater than lesser.
typedef bool (*quicksort_sorter_float)(const float* greater,
									   const float* lesser);

/// Sort some arbitrary, uniformly-spaced array of floats
/// @param target: first float in array
/// @param nmemb: number of floats in the array
/// @param stride: distance in bytes between each float (sizeof(float) for
/// contiguous array)
/// @param handler: function that gets called whenever an element in the array
/// is moved.
/// @param user_data: pointer propagated to the handler
/// @param sorter: function that gets called to compare two elements in the
/// array.
void quicksort_inplace_generic_sort_handler_float(
	float* target, quicksort_index_t nmemb, uint8_t stride,
	quicksort_swap_handler_t handler, void* user_data,
	quicksort_sorter_float sorter);
