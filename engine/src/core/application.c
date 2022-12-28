#include "application.h"
#include "game_types.h"

#include "platform/platform.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"

#include "renderer/renderer_frontend.h"

typedef struct application_state {
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;
    clock clock;
} application_state;

static b8 initialized = FALSE;
static application_state app_state;

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context data);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context data);

b8 application_create(game* game_inst){
    if(initialized) {
	LOG_ERROR("application_create called more than once.")
	return FALSE;
    }

    app_state.game_inst = game_inst;

    // initializing subsystems...
    log_init();
    input_initialize();

    app_state.is_running   = TRUE;
    app_state.is_suspended = FALSE;

    if(!event_initialize()){
	LOG_ERROR("event system failed initialization. Application cannot continue.");
	return FALSE;
    }

    event_register(EVENT_CODE_APPLICATION_QUIT,	0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 	0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 	0, application_on_key);

    if(!platform_startup(
	    &app_state.platform,
	    game_inst->app_config.name, 
	    game_inst->app_config.start_pos_x,
	    game_inst->app_config.start_pos_y, 
	    game_inst->app_config.start_width, 
	    game_inst->app_config.start_height)) {
	return FALSE;
    }

    // Renderer startup.
    if (!renderer_initialize(game_inst->app_config.name, &app_state.platform)) {
	LOG_FATAL("Failed to initialize renderer. Aborting application.");
	return FALSE;
    }

    // Initialize the game.
    if(!game_inst->initialize(game_inst)) {
	LOG_FATAL("Game failed to initialize.");
	return FALSE;
    }

    game_inst->on_resize(game_inst, app_state.width, app_state.height);

    initialized = TRUE;
    return TRUE;
}

b8 application_run() {
    clock_start(&app_state.clock);
    clock_update(&app_state.clock);
    app_state.last_time = app_state.clock.elapsed;

    u8 frame_count        = 0;
    f64 running_time      = 0;
    f64 target_frame_time = 1.0f/60;

    LOG_INFO(get_memory_usage_str());

    while(app_state.is_running) {

	if(!platform_pump_messages(&app_state.platform)) {
	    app_state.is_running = FALSE;
	}
	if(!app_state.is_suspended) {

	    // frame start calculation
	    clock_update(&app_state.clock);
	    f64 current_time     = app_state.clock.elapsed;
	    f64 delta            = current_time - app_state.last_time;
	    f64 frame_start_time = platform_get_absolute_time();

	    if(!app_state.game_inst->update(app_state.game_inst, (f32)delta)){
		LOG_FATAL("game update failed, shutting down");
		app_state.is_running = FALSE;
		break;
	    }
	    if(!app_state.game_inst->render(app_state.game_inst, (f32)delta)){
		LOG_FATAL("game render failed, shutting down");
		app_state.is_running = FALSE;
		break;
	    }

	    //TODO: refactor packet creation.
	    render_packet packet;
	    packet.delta_time = delta;
	    renderer_draw_frame(&packet);

	    // frame start calculation
	    f64 frame_end_time = platform_get_absolute_time();
	    f64 elaped_frame_time = frame_end_time - frame_start_time;
	    running_time += elaped_frame_time;
	    f64 remaining_frame_time_s = target_frame_time - elaped_frame_time;
	    // I guess if positive we drop a frame.
	    

	    // FIXME: limiting frames hardcoded to FALSE ! 
	    if(remaining_frame_time_s > 0) {
		u64 remaining_frame_time_ms = remaining_frame_time_s * 1000;
		b8 limit_frames = FALSE;
		if(remaining_frame_time_ms > 0 && limit_frames) {
		    platform_sleep(remaining_frame_time_ms - 1);
		}
		frame_count ++;
	    }

	    // input updated last to be processed on next frame.
	    input_update(delta);

	    app_state.last_time = current_time;
	}
    }
    app_state.is_running = FALSE;

    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 	  0, application_on_key);
    event_unregister(EVENT_CODE_BUTTON_RELEASED,  0, application_on_key);

    event_shutdown();
    input_shutdown();
    renderer_shutdown();
    platform_shutdown(&app_state.platform);
    return TRUE;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context){
    switch(code) {
	case EVENT_CODE_APPLICATION_QUIT: {
	    LOG_INFO("EVENT_CODE_APPLICATION_QUIT received, shutting down\n");
	    app_state.is_running = FALSE;
	    return TRUE; // return true so that no other listerner can process event. 
	}
    }
    return FALSE;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context){
    u16 key_code = context.data.u16[0];
    if(code == EVENT_CODE_KEY_PRESSED) {
		if(key_code == KEY_ESCAPE) {
	    	event_context data = {};
	    	event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
	    	return TRUE;
		} else if(key_code == KEY_A){
	    	LOG_DEBUG("Explicit - A key pressed.");
		} else {
	    LOG_DEBUG("%c key pressed in window.", key_code);
		}
    } else if (code == EVENT_CODE_KEY_RELEASED) {
		if(key_code == KEY_B) {
	    	LOG_DEBUG("Explicit - B key pressed.");
		} else {
	    	LOG_DEBUG("%c key released in window.", key_code);
		}
    }
    return FALSE;
}
