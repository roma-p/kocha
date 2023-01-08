#include "renderer_frontend.h"
#include "renderer_backend.h"

#include "core/logger.h"
#include "core/kmemory.h"

static renderer_backend* backend = 0; // ??? why pointer here when other static struct are declared as plain data?

b8 renderer_initialize(const char * application_name, struct platform_state* plat_state){
    backend = kallocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);

    //TODO: make this configuratable.
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state, backend);
    backend->frame_number = 0;

    if(!backend->initialize(backend, application_name, plat_state)){
    LOG_FATAL("Renderer backend failed to initialize. Shutting gown.");
    return FALSE;
    }

    return TRUE;
}

void renderer_shutdown() {
    backend->shutdown(backend);
    kfree(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
}

b8 renderer_begin_frame(f32 delta_time){
    return backend->begin_frame(backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time){
    b8 result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

b8 renderer_draw_frame(render_packet* packet){
    // if hte begin frame returned sucessfully, mid-frame operationsmay continue.
    if (renderer_begin_frame(packet->delta_time)) {

    //end the frame, if this fails, likely uncoverable.
    b8 result = renderer_end_frame(packet->delta_time);

    if(!result) {
        LOG_ERROR("renderer_end_frame failed. Application shutting down");
        return FALSE;
    }
    }
    return TRUE;
}
