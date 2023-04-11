///
/// Basic dynamically allocated array implementation. Defining DYNARRAY_TYPE
/// before including this header will change the expected type.
///

#ifndef _DYNARRAY_HEADER
#define _DYNARRAY_HEADER

#include "architecture.h"
#include "shorthand.h"

#ifndef DYNARRAY_TYPE
#define DYNARRAY_TYPE GameObject
#endif

typedef struct Dynarray
{
	DYNARRAY_TYPE *head;
	uint size;
	uint capacity;
} Dynarray;

Dynarray dynarray_create(int initial_capacity);
void dynarray_insert(Dynarray *dynarray, DYNARRAY_TYPE new);
void dynarray_remove(Dynarray *dynarray, uint index);

///
/// This provides the implementation of the functions in this header.
/// Implementing a dynamic array should only be done once for a certain
/// project.
///
#ifdef _IMPLEMENT_DYNARRAY
Dynarray dynarray_create(int initial_capacity)
{
	GameObject *head = malloc(initial_capacity * sizeof(DYNARRAY_TYPE));
	if (head == NULL) {
		printf("Memory allocation for dynamic array of capacity %d failed.\n",
			   initial_capacity);
		exit(EXIT_FAILURE);
	}

	return (Dynarray){.head = head, .capacity = initial_capacity, .size = 0};
}

void dynarray_insert(Dynarray *dynarray, DYNARRAY_TYPE new)
{
	if (dynarray->size >= dynarray->capacity) {
		// we dont have enough space, realloc
		uint new_capacity = dynarray->capacity * 2;
		DYNARRAY_TYPE *new_head =
			malloc((new_capacity) * sizeof(DYNARRAY_TYPE));
		if (new_head == NULL) {
			printf("Memory re-allocation failed for dynamic array of old size "
				   "%d and new size %d.\n",
				   dynarray->capacity, new_capacity);
			exit(EXIT_FAILURE);
		}

		// copy over all of the old contents
		memcpy(new_head, dynarray->head, dynarray->size);

		dynarray->head = new_head;
		dynarray->capacity = new_capacity;
	}
	assert(dynarray->size < dynarray->capacity);
	(*dynarray->head[dynarray.size]) = new;
	dynarray->size += 1;
}

void dynarray_remove(Dynarray *dynarray, uint index) {
    
}
#endif

#endif
