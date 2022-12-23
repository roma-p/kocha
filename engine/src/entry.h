#pragma once
#include "core/application.h"
#include "core/logger.h"
#include "core/kmemory.h"
#include "game_types.h"

extern b8 create_game(game* out_game);

int main(void) {

    // only subsystem mananged at entry level. The other at application level.
    initialize_memory();


    game game_inst;
    if(!create_game(&game_inst)){
	LOG_FATAL("Could not create game.");
	return -1;
    }

    if(!game_inst.render || !game_inst.update || !game_inst.initialize || !game_inst.on_resize) {
	LOG_FATAL("Some of game instance function not assigned");
	return -2;
    }

    // INIT 
    if(!application_create(&game_inst)){
	LOG_INFO("Application failed to create.");
	return 1;
    }

    // BEGIN GAME LOOP.
    if(!application_run()) {
	LOG_INFO("Application did not shutdown successfully");
	return 2;
    }

    shutdown_memory();

    return 0;
}
