#pragma once
#include "raylib.h"
#include "architecture.h"
#include "gameobject.h"

// Like a constructor:
// Hook up all the GameObject optional features
// Attach the chosen inputs to the update method
GameObject create_airplane(Gamestate gamestate, uint player);
