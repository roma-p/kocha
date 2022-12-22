#include "application.h"
#include "logger.h"
#include "platform/platform.h"
#include "game_types.h"

typedef struct application_state {
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;
} application_state;

static b8 initialized = FALSE;
static application_state app_state;

b8 application_create(game* game_inst){
    if(initialized) {
	LOG_ERROR("application_create called more than once.")
	return FALSE;
    }

    app_state.game_inst = game_inst;

    log_init();
    LOG_FATAL("C'est la merde %f", 3.14f);
    LOG_ERROR("C'est la merde %d", 4);
    LOG_WARN("C'est la merde %d", 4);
    LOG_INFO("C'est la merde %d", 4);
    LOG_DEBUG("C'est la merde %d", 4);
    LOG_TRACE("C'est la merde %d", 4);

    app_state.is_running   = TRUE;
    app_state.is_suspended = FALSE;

    if(!platform_startup(
	    &app_state.platform,
	    game_inst->app_config.name, 
	    game_inst->app_config.start_pos_x,
	    game_inst->app_config.start_pos_y, 
	    game_inst->app_config.start_width, 
	    game_inst->app_config.start_height)) {
	return FALSE;
    }

    if(!game_inst->initialize(game_inst)) {
	LOG_FATAL("Game failed to initialize.");
	return FALSE;
    }

    game_inst->on_resize(game_inst, app_state.width, app_state.height);

    initialized = TRUE;
    return TRUE;
}

b8 application_run() {
    while(app_state.is_running) {
		if(!platform_pump_messages(&app_state.platform)) {
	    	app_state.is_running = FALSE;
		}
		if(!app_state.is_suspended) {
		    if(!app_state.game_inst->update(app_state.game_inst, (f32)0)){
				LOG_FATAL("game update failed, shutting down");
				app_state.is_running = FALSE;
				break;
		    }
		    	if(!app_state.game_inst->render(app_state.game_inst, (f32)0)){
				LOG_FATAL("game render failed, shutting down");
				app_state.is_running = FALSE;
				break;
		    }
		}
    }
    app_state.is_running = FALSE;
    platform_shutdown(&app_state.platform);
    return TRUE;
}
