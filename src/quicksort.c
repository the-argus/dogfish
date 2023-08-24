#include "quicksort.h"
#include <assert.h>
#include <stddef.h>

#define ARRAY_STRIDE(type, head, stride, index) \
	((type*)((uint8_t*)head + ((ptrdiff_t)index * stride)))

#define QUICKSORT_PRIVATE_DEF(type)                                           \
	void quicksort_private_inplace_##type(                                    \
		type* target, uint8_t stride, uint16_t left_max, uint16_t right_max)  \
	{                                                                         \
		if (left_max >= right_max) {                                          \
			return;                                                           \
		}                                                                     \
                                                                              \
		uint16_t left_iter = left_max;                                        \
		uint16_t right_iter = right_max;                                      \
		type* pivot = ARRAY_STRIDE(type, target, stride, left_max);           \
		const type pivot_val = *pivot;                                        \
                                                                              \
		while (1) {                                                           \
			/* walk in from left to right */                                  \
			for (; left_iter < right_max; ++left_iter) {                      \
				if (*ARRAY_STRIDE(type, target, stride, left_iter) >          \
					pivot_val) {                                              \
					break;                                                    \
				}                                                             \
			}                                                                 \
                                                                              \
			/* from right to left now! */                                     \
			for (; right_iter > left_max; --right_iter) {                     \
				if (*ARRAY_STRIDE(type, target, stride, right_iter) <=        \
					pivot_val) {                                              \
					break;                                                    \
				}                                                             \
			}                                                                 \
                                                                              \
			if (left_iter < right_iter) {                                     \
				/* swap */                                                    \
				/* TODO: use xor? */                                          \
				type* left = ARRAY_STRIDE(type, target, stride, left_iter);   \
				type* right = ARRAY_STRIDE(type, target, stride, right_iter); \
				const type temp = *left;                                      \
				*left = *right;                                               \
				*right = temp;                                                \
			} else {                                                          \
				break;                                                        \
			}                                                                 \
		}                                                                     \
                                                                              \
		/* swap pivot and right */                                            \
		type* right = ARRAY_STRIDE(type, target, stride, right_iter);         \
		*pivot = *right;                                                      \
		*right = pivot_val;                                                   \
                                                                              \
		/* recurse */                                                         \
		if (right_iter != 0) {                                                \
			quicksort_private_inplace_##type(target, stride, left_max,        \
											 right_iter - 1);                 \
		}                                                                     \
		quicksort_private_inplace_##type(target, stride, right_iter + 1,      \
										 right_max);                          \
	}

// NOLINTNEXTLINE
QUICKSORT_PRIVATE_DEF(float);
// NOLINTNEXTLINE
QUICKSORT_PRIVATE_DEF(uint16_t);

void quicksort_inplace_float(float* target, uint16_t nmemb, uint8_t stride)
{
	if (nmemb == 0) {
		return;
	}
	quicksort_private_inplace_float(target, stride, 0, nmemb - 1);
}

void quicksort_inplace_uint16(uint16_t* target, uint16_t nmemb, uint8_t stride)
{
	if (nmemb == 0) {
		return;
	}
	quicksort_private_inplace_uint16_t(target, stride, 0, nmemb - 1);
}
