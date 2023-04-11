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

typedef 
