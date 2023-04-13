#include "object_structure.h"
#include "architecture.h"

#define DYNARRAY_TYPE GameObject
#define DYNARRAY_TYPE_NAME Dynarray_GameObject
#include "dynarray.h"

void object_structure_insert(ObjectStructure *structure, GameObject new)
{
	dynarray_insert_GameObject(&structure->_dynarray, new);
}

void object_structure_remove(ObjectStructure *structure, uint index)
{
	// call the object's cleanup function if it has one
	if (structure->_dynarray.head[index].cleanup.has) {
		structure->_dynarray.head[index].cleanup.value(
			&structure->_dynarray.head[index]);
	}
	dynarray_remove_GameObject(&structure->_dynarray, index);
}

ObjectStructure object_structure_create()
{
	Dynarray_GameObject array = dynarray_create_GameObject(20);
	return (ObjectStructure){._dynarray = array};
}

void object_structure_map(ObjectStructure *structure,
						  void (*map_func)(GameObject *self, uint index))
{
	dynarray_map_GameObject(&structure->_dynarray, map_func);
}

int object_structure_remove_by_id(ObjectStructure *structure, ushort id)
{
	for (uint i = 0; i < structure->_dynarray.size; i++) {
		if (structure->_dynarray.head[i].id == id) {
			// game object with matching id found
			dynarray_remove_GameObject(&structure->_dynarray, i);
			return 1;
		}
	}
	return 0;
}

int object_structure_size(ObjectStructure *object)
{
	return object->_dynarray.size;
}
