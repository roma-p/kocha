#include "defines.h"
#include "core/logger.h"
#include "containers/darray.h"
#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.inl"

static vulkan_context context;

b8 vulkan_renderer_backend_initialize(
	    renderer_backend* backend,
	    const char* application_name,
	    struct platform_state* plat_state){

    // TODO: custom allocator ! 
    context.allocator = 0;

    //app_info created on stack because once vkinstance created, no need for app_info ???

    const char** required_extensions = darray_create(const char*);

    // TODO: put this in platform layer somehow.
    #if KPLATFORM_APPLE == 1
    darray_push(required_extensions, &VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    #endif

    //setup vulkan instance.
    VkApplicationInfo app_info  = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion         = VK_API_VERSION_1_2;
    app_info.pApplicationName   = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "Kocha Enine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info    = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = darray_length(required_extensions);
    create_info.enabledLayerCount       = 0;
    create_info.ppEnabledExtensionNames = required_extensions;
    create_info.ppEnabledLayerNames     = 0;

    #if KPLATFORM_APPLE == 1
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif




    VkResult result = vkCreateInstance(&create_info, context.allocator, &context.instance);
    if (result != VK_SUCCESS){
	LOG_ERROR("vkcreateInstance failed with result: %d", result);
	return FALSE;
    }
    LOG_INFO("vulkan renderer initialized sucessfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend){

}

void vulkan_renderer_backend_on_resize(renderer_backend* backend, u16 width, u16 height){

}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time){
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time){
    return TRUE;
}

