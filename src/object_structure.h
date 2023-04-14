#pragma once

///
/// This is an abstract, unsorted container for GameObjects. It should be
/// capable of insertion, removal, and iteration. At the moment this is
/// implemented by dynamic array. In the future, GameObjects should be able
/// to specify whether to optimize for common insertion/deletion or not, and if
/// they do then a pool will be created for them.
///
/// Iteration does not necessarily occur in insertion order.
///

#include "architecture.h"
#include "shorthand.h"

// define game object dynamic array
#define DYNARRAY_TYPE GameObject
#define DYNARRAY_TYPE_NAME Dynarray_GameObject
#include "dynarray.h"

typedef struct ObjectStructure
{
	Dynarray_GameObject _dynarray;
} ObjectStructure;

void object_structure_insert(ObjectStructure *structure, GameObject new);
void object_structure_remove(ObjectStructure *structure, uint index);
int object_structure_remove_by_id(ObjectStructure *structure, ushort id);
ObjectStructure object_structure_create();
void object_structure_map(ObjectStructure *structure,
						  void (*map_func)(GameObject *self, uint index));
int object_structure_size(ObjectStructure *object);
void object_structure_destroy(ObjectStructure *structure);
