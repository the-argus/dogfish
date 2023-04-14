///
/// Basic dynamically allocated array implementation. Defining DYNARRAY_TYPE
/// before including this header will change the expected type.
///

#ifndef _DYNARRAY_HEADER
#define _DYNARRAY_HEADER

#include "gameobject.h"
#include "shorthand.h"
#include <assert.h>

#ifndef DYNARRAY_TYPE
#define DYNARRAY_TYPE GameObject
#ifdef DYNARRAY_TYPE_NAME
// clang-format off
#warning "DYNARRAY_TYPE_NAME defined but not DYNARRAY_TYPE... using DYNARRAY_TYPE_NAME for GameObject dynamic array."
// clang-format on
#else
#define DYNARRAY_TYPE_NAME Dynarray_GameObject
#endif
#else
#ifndef DYNARRAY_TYPE_NAME
#error \
	"DYNARRAY_TYPE defined, but DYNARRAY_TYPE_NAME not defined. Please define both."
#endif
#endif

// funny trick to call a macro with macro args
#define _CALL(macro, ...) macro(__VA_ARGS__)

typedef struct DYNARRAY_TYPE_NAME
{
	DYNARRAY_TYPE *head;
	uint size;
	uint capacity;
} DYNARRAY_TYPE_NAME;

#define _DYNARRAY_CREATE_SIGNATURE(type) \
	DYNARRAY_TYPE_NAME dynarray_create_##type(int initial_capacity)

#define _DYNARRAY_DESTROY_SIGNATURE(type) \
	void dynarray_destroy_##type(DYNARRAY_TYPE_NAME *dynarray)

#define _DYNARRAY_INSERT_SIGNATURE(type) \
	void dynarray_insert_##type(DYNARRAY_TYPE_NAME *dynarray, type new)

#define _DYNARRAY_REMOVE_SIGNATURE(type) \
	void dynarray_remove_##type(DYNARRAY_TYPE_NAME *dynarray, uint index)

#define _DYNARRAY_MAP_SIGNATURE(type) \
	void dynarray_map_##type(         \
		DYNARRAY_TYPE_NAME *dynarray, \
		void (*map_func)(DYNARRAY_TYPE * item, uint index))

#define _DYNARRAY_FUNCTION_DECLARATIONS(type) \
	_DYNARRAY_CREATE_SIGNATURE(type);         \
	_DYNARRAY_DESTROY_SIGNATURE(type);        \
	_DYNARRAY_INSERT_SIGNATURE(type);         \
	_DYNARRAY_REMOVE_SIGNATURE(type);         \
	_DYNARRAY_MAP_SIGNATURE(type);

_DYNARRAY_FUNCTION_DECLARATIONS(DYNARRAY_TYPE)

///
/// This provides the implementation of the functions in this header.
/// Implementing a dynamic array should only be done once for a certain
/// project.
///
#ifdef _IMPLEMENT_DYNARRAY
_CALL(_DYNARRAY_CREATE_SIGNATURE, DYNARRAY_TYPE)
{
	DYNARRAY_TYPE *head = malloc(initial_capacity * sizeof(DYNARRAY_TYPE));
	if (head == NULL) {
		printf("Memory allocation for dynamic array of capacity %d failed.\n",
			   initial_capacity);
		exit(EXIT_FAILURE);
	}

	return (DYNARRAY_TYPE_NAME){
		.head = head, .capacity = initial_capacity, .size = 0};
}

_CALL(_DYNARRAY_INSERT_SIGNATURE, DYNARRAY_TYPE)
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
		memcpy(new_head, dynarray->head,
			   dynarray->size * sizeof(DYNARRAY_TYPE));

		free(dynarray->head);
		dynarray->head = new_head;
		dynarray->capacity = new_capacity;
	}
	assert(dynarray->size < dynarray->capacity);
	dynarray->head[dynarray->size] = new;
	dynarray->size += 1;
}

_CALL(_DYNARRAY_REMOVE_SIGNATURE, DYNARRAY_TYPE)
{
	// assert that index is in range
	if (index >= dynarray->size) {
		printf("Index %d out of range of dynamic array of size %d\n", index,
			   dynarray->size);
		exit(EXIT_FAILURE);
	}

	// perform a memcpy on each induvidual item
	// to move everything after index back one
	for (uint i = index; i < dynarray->size; i++) {
		// move current index to index-1
		memcpy(&dynarray->head[index], &dynarray->head[index + 1],
			   sizeof(DYNARRAY_TYPE));
	}

	// reduce size
	dynarray->size -= 1;
}

_CALL(_DYNARRAY_MAP_SIGNATURE, DYNARRAY_TYPE)
{
	for (uint i = 0; i < dynarray->size; i++) {
		map_func(&(dynarray->head[i]), i);
	}
}

_CALL(_DYNARRAY_DESTROY_SIGNATURE, DYNARRAY_TYPE) { free(dynarray->head); }

#endif

#endif
