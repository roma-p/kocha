#include <core/logger.h>
#include <core/assert.h>
#include <platform/platform_macos.m>


int main(void) {
    LOG_FATAL("C'est la merde %f", 3.14f);
    LOG_ERROR("C'est la merde %d", 4);
    LOG_WARN("C'est la merde %d", 4);
    LOG_INFO("C'est la merde %d", 4);
    LOG_DEBUG("C'est la merde %d", 4);
    LOG_TRACE("C'est la merde %d", 4);
    
    platform_state state;
    if(platform_startup(&state, "KOCHA TEST", 100, 100, 1280, 720)) {
	while(TRUE) {
	    platform_pump_messages(&state);
	}
    }
    platform_shutdown(&state);

    return 0;
}
