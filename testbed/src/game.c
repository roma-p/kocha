#include "game.h"
#include <core/logger.h>

b8 game_initialize(struct game* game_inst){
    LOG_DEBUG("game initialized")
    return TRUE;
}

b8 game_update(struct game* game_inst, f32 delta_time){
    return TRUE;
}

b8 game_render(struct game* game_inst, f32 delta_time){
    return TRUE;
}

// FORWARD DECLARE OF struct game* game_inst to avoid circular dependency. 
// but not enterely understood...
void game_resize(struct game* game_inst, u32 width, u32 height){
}

