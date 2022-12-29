#include "defines.h"
#include "core/logger.h"
#include "core/kstring.h"
#include "containers/darray.h"
#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/vulkan_platform.h"

static vulkan_context context;

b8 vulkan_renderer_backend_initialize(
	    renderer_backend* backend,
	    const char* application_name,
	    struct platform_state* plat_state){

    // TODO: custom allocator ! 
    context.allocator = 0;

    // EXTENSIONS ------------------------------------------------------------
    const char** required_extensions = darray_create(const char*);
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);
    platform_get_required_extension_names(&required_extensions);

    #if defined(_DEBUG)
	darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	LOG_DEBUG("Required vulkan extensions are:");
	i32 i; i32 count = darray_length(required_extensions);
	for(i=0; i<count; i++){
	    LOG_DEBUG(" - %s", required_extensions[i]);
	}
    #endif

    // VAlIDATION LAYERS -----------------------------------------------------
    const char** required_layers = 0;
    u32 required_layers_count    = 0;
    #if defined(_DEBUG)
	required_layers = darray_create(const char*);
	darray_push(required_layers, &"VK_LAYER_KHRONOS_validation");
	required_layers_count = darray_length(required_layers);

	u32 available_layer_count; VkResult vk_result;

	vk_result = vkEnumerateInstanceLayerProperties(&available_layer_count, 0);
	VK_CHECK(vk_result, "failed to list instance layer properties");
	VkLayerProperties* available_layers = darray_reserve(available_layer_count, sizeof(VkLayerProperties));
	vk_result = vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);
	VK_CHECK(vk_result, "failed to list instance layer properties");

	int j; char *layer;
	b8 layer_found; b8 all_layers_found = TRUE;
	for(i=0; i<required_layers_count; i++) {
	    const char * layer = required_layers[i];
	    layer_found = FALSE;
	    for(j=0; j<available_layer_count; j++){
		if(strings_equal(layer, available_layers[j].layerName)){
		    layer_found = TRUE;
		    break;
		}
	    }
	    if(!layer_found){
		LOG_ERROR("Validation layer not available: %s", layer);
		all_layers_found = FALSE;
	    }
	}
	if(all_layers_found == FALSE){
	    LOG_ERROR("Some validation layers are missing.");
	    return FALSE;
	}

	LOG_DEBUG("Required vulkan validation layers are:");
	for(i=0; i<required_layers_count; i++){
	    LOG_DEBUG(" - %s", required_layers[i]);
	}

    #endif	
    

    // APP INFO --------------------------------------------------------------
    VkApplicationInfo app_info  = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion         = VK_API_VERSION_1_2;
    app_info.pApplicationName   = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "Kocha Enine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);

    // CREATE INFO -----------------------------------------------------------
    VkInstanceCreateInfo create_info    = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = darray_length(required_extensions);
    create_info.enabledLayerCount       = required_layers_count;
    create_info.ppEnabledExtensionNames = required_extensions;
    create_info.ppEnabledLayerNames     = required_layers;
    // TODO: deport me in platform_macos.m
    #if KPLATFORM_APPLE == 1
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    // VKINSTANCE CREATION ---------------------------------------------------
    VkResult result = vkCreateInstance(&create_info, context.allocator, &context.instance);

    darray_destroy(required_extensions);
    
    VK_CHECK(result, "vkCreateInstance failed.");
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

