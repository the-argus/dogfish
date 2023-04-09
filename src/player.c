
#include "architecture.h"
#include "ode/ode.h"


void update(struct GameObject *self, Gamestate *gamestate, float delta_time) {
    if (self->physics.has) {
        dCollide(self->physics.value,);
    }

    gamestate->p1_camera;
}

GameObject create_player(Gamestate *gamestate) {

    GameObject player = {.physics = {.has = 1, .value = dBodyCreate()},
        .update = {.has = 1, .value = &update}
    };
}
