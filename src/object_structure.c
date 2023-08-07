#include "object_structure.h"

void object_structure_insert(ObjectStructure *structure, GameObject new)
{
	dynarray_insert_GameObject(&structure->_dynarray, new);
}

void object_structure_remove(ObjectStructure *structure, size_t index)
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
	Dynarray_GameObject queue = dynarray_create_GameObject(20);
	return (ObjectStructure){._dynarray = array, ._create_queue = queue};
}

void object_structure_destroy(ObjectStructure *structure)
{
	for (int i = 0; i < object_structure_size(structure); i++) {
		if (structure->_dynarray.head[i].cleanup.has) {
			structure->_dynarray.head[i].cleanup.value(
				&structure->_dynarray.head[i]);
		}
	}
	dynarray_destroy_GameObject(&structure->_dynarray);
	dynarray_destroy_GameObject(&structure->_create_queue);
}

void object_structure_map(ObjectStructure *structure,
						  void (*map_func)(GameObject *self, size_t index))
{
	dynarray_map_GameObject(&structure->_dynarray, map_func);
}

int object_structure_remove_by_id(ObjectStructure *structure, uint16_t id)
{
	for (size_t i = 0; i < structure->_dynarray.size; i++) {
		if (structure->_dynarray.head[i].id == id) {
			// game object with matching id found
			dynarray_remove_GameObject(&structure->_dynarray, i);
			return 1;
		}
	}
	return 0;
}

size_t object_structure_size(ObjectStructure *object)
{
	return object->_dynarray.size;
}

// add everything in the creation queue to the actual array
void object_structure_flush_create_queue(ObjectStructure *structure)
{
	size_t initial_size = structure->_create_queue.size;

	if (initial_size == 0) {
		return;
	}

	for (size_t i = initial_size - 1; i >= 0; --i) {
		object_structure_insert(structure, structure->_create_queue.head[i]);
		dynarray_remove_GameObject(&structure->_create_queue, i);
	}
}

void object_structure_queue_for_creation(ObjectStructure *structure,
										 GameObject object)
{
	dynarray_insert_GameObject(&structure->_create_queue, object);
}
