#include "defines.h"
#include "core/logger.h"
#include "core/assert.h"
#include "core/kstring.h"
#include "core/kmemory.h"
#include "containers/darray.h"

#include "core/application.h"

#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/vulkan_platform.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_fence.h"
#include "vulkan/vulkan_core.h"

static vulkan_context context;
static u32 cached_framebuffer_width  = 0;
static u32 cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);

void create_command_buffers(renderer_backend* backend);
void regenerate_frame_buffers(
    renderer_backend* backend,
    vulkan_swapchain* swapchain,
    vulkan_renderpass* renderpass
);
b8 recreate_swapchain(renderer_backend* backend);

b8 vulkan_renderer_backend_initialize(
        renderer_backend* backend,
        const char* application_name,
        struct platform_state* plat_state){

    context.find_memory_index = find_memory_index;

    // TODO: custom allocator ! 
    context.allocator = 0;

    // BUG HERE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    application_get_framebuffer_size(
            &cached_framebuffer_width,
            &cached_framebuffer_height
        );
    context.framebuffer_width  = (cached_framebuffer_width != 0)  ? cached_framebuffer_width  : 800;
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    cached_framebuffer_width  = 0;
    cached_framebuffer_height = 0;

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

    // VULKAN DEBUGGER -------------------------------------------------------

    #if defined(_DEBUG)

    u32 severity_level = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | \
                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    u32 message_types  = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | \
                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | \
                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    // debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = severity_level;
    debug_info.messageType     = message_types;
    debug_info.pfnUserCallback = vk_debug_callback;
    // debug_info.pUserData       = 0;
    
    // some vk extension not loaded automatically, so we need to call a vulkan API to set 
    // a func pointer defined by us to point to the desired extension API.
    
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            context.instance,
            "vkCreateDebugUtilsMessengerEXT"
        );
    const char * vulkan_debug_mess_error =  "error creating vulkan debugger messenger";
    KASSERT_MSG(func, vulkan_debug_mess_error);
    VK_CHECK(
        func(
        context.instance,
        &debug_info,
        context.allocator,
        &context.debug_messenger
        ),
        vulkan_debug_mess_error
    );
    LOG_INFO("vulkan debugger successfully instanciated");
    #endif

    // SURFACE ---------------------------------------------------------------
    LOG_DEBUG("creating vulkan surface");
    if(!platform_create_vulkan_surface(plat_state, &context)){
        LOG_ERROR("failed to create platform surface");
        return FALSE;
    }
    LOG_INFO("vulkan surface initialized sucessfully.");


    // DEVICE CREATION -------------------------------------------------------
    LOG_DEBUG("creating device");
    if(!vulkan_device_create(&context)) {
        LOG_ERROR("failed to create vulkan device");
        return FALSE;
    }

    // SWAPCHAIN CREATION ----------------------------------------------------
    LOG_DEBUG("creating swapchain");
    vulkan_swapchain_create(
            &context,
            context.framebuffer_width,
            context.framebuffer_height,
            &context.swapchain
        );

    // RENDERPASS CREATION ---------------------------------------------------
    LOG_DEBUG("creating render pass");
    vulkan_renderpass_create(
            &context,
            &context.main_renderpass,
            0, 0, context.framebuffer_width, context.framebuffer_height,
            0.0f, 0.0f, 0.0f, 1.0f, // clear values defined here
            1.0f,
            0);


    // FRAME BUFFER CREATION -------------------------------------------------
    context.swapchain.framebuffers = darray_reserve(
            vulkan_framebuffer,
            context.swapchain.image_count
        );
    regenerate_frame_buffers(backend, &context.swapchain, &context.main_renderpass);

    // COMMAND BUFFER CREATION -----------------------------------------------
    LOG_DEBUG("creating command buffers");
    create_command_buffers(backend);

    // SYNC OBJECTS CREATION -------------------------------------------------
    LOG_DEBUG("creating sync objects");
    context.image_available_semaphore = darray_reserve(
            VkSemaphore,
            context.swapchain.max_frames_in_flight
        );
    context.queue_complete_semaphore = darray_reserve(
            VkSemaphore,
            context.swapchain.max_frames_in_flight
        );
    context.in_flight_fences = darray_reserve(
            vulkan_fence,
            context.swapchain.max_frames_in_flight
        );
    for(u8 i = 0; i<context.swapchain.max_frames_in_flight; i++ ){
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(
                context.device.logical_device, 
                &semaphore_create_info,
                context.allocator,
                &context.image_available_semaphore[i]
            );
        vkCreateSemaphore(
                context.device.logical_device, 
                &semaphore_create_info,
                context.allocator,
                &context.queue_complete_semaphore[i]
            );
        // first image can't be rendered (for some reason?)
        // so tagging semaphore as "signaled".
        vulkan_fence_create(&context, TRUE, &context.in_flight_fences[i]);
    }

    context.image_in_flight = darray_reserve(
            vulkan_fence,
            context.swapchain.image_count
        );
    // Not image in flight at start of engine, so forcing 0 in this pointer array
    // a zero pointer means that the image at this index is not in flight.
    for(u32 i =0; i< context.swapchain.image_count; ++i) {
        context.image_in_flight[i] = 0;
    }

    LOG_INFO("vulkan renderer initialized sucessfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend){
    LOG_DEBUG("Destroying Vulkan sync objects...");

    // waiting no more graphics operations
    vkDeviceWaitIdle(context.device.logical_device);
    for(u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        if (context.image_available_semaphore[i]) {
            vkDestroySemaphore(
                    context.device.logical_device,
                    context.image_available_semaphore[i],
                    context.allocator
                );
            context.image_available_semaphore[i] = 0;
        }
        if (context.queue_complete_semaphore[i]) {
            vkDestroySemaphore(
                    context.device.logical_device,
                    context.queue_complete_semaphore[i],
                    context.allocator
                );
            context.queue_complete_semaphore[i] = 0;
        }
        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
    }
    darray_destroy(context.image_available_semaphore);
    context.image_available_semaphore = 0;

    darray_destroy(context.queue_complete_semaphore);
    context.queue_complete_semaphore = 0;

    darray_destroy(context.in_flight_fences);
    context.in_flight_fences = 0;

    LOG_DEBUG("Destroying Vulkan frame buffers...");
    for(u32 i = 0; i<context.swapchain.image_count; i++){
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }
    
    LOG_DEBUG("Destroying Vulkan command buffers...");
    for(u32 i = 0; i<context.swapchain.image_count; i++){
        if(context.graphics_command_buffers[i].handle) {
            vulkan_command_buffer_free(
                    &context,
                    context.device.graphics_command_pool,
                    &context.graphics_command_buffers[i]
                );
            context.graphics_command_buffers[i].handle = 0;
        }
    }
    darray_destroy(context.graphics_command_buffers);
    context.graphics_command_buffers = 0;

    LOG_DEBUG("Destroying Vulkan renderpass...");
    vulkan_renderpass_destroy(&context, &context.main_renderpass);

    LOG_DEBUG("Destroying Vulkan swapchain...");
    vulkan_swapchain_destroy(&context, &context.swapchain);

    LOG_DEBUG("Destroying Vulkan device...")
    vulkan_device_destroy(&context);

    LOG_DEBUG("Destroying Vulkan surface...")
    if(context.surface) {
        vkDestroySurfaceKHR(
                context.instance,
                context.surface,
                context.allocator
            );
        context.surface = 0;
    }

    if (context.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            context.instance,
            "vkDestroyDebugUtilsMessengerEXT"
        );
        func(context.instance, context.debug_messenger, context.allocator);
    }
    LOG_DEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resize(renderer_backend* backend, u16 width, u16 height){
    cached_framebuffer_width  = width;
    cached_framebuffer_height = height;
    context.framebuffer_size_generation ++;

    LOG_INFO("vulkan renderer backend: w/h/gen: %i/%i/%llu", 
            width, height, context.framebuffer_size_last_generation
        );
}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time){
    vulkan_device* device = &context.device;

    // checking swapchain recreation not in progress -------------------------
    if(context.recreating_swapchain) {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if(!vulkan_result_is_success(result)) {
            LOG_ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, TRUE)
                );
            return FALSE;
        }
        LOG_INFO("Recreating swapchain, booting.");
        return FALSE;
    }

    // checking window has not been resized, if so new swapchain needed.------
    if(context.framebuffer_size_last_generation != context.framebuffer_size_generation) {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if(!vulkan_result_is_success(result)) {
            LOG_ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, TRUE)
                );
            return FALSE;
        }

        // recreate swapchain can fail cause for example window has been minimized.
        // Then, exit this func and try to create the swapchain on next frame.
        if(!recreate_swapchain(backend)) {
            return FALSE;
        }
        LOG_INFO("Rezied, booting.")
        return FALSE;
    }

    // waiting for the execution of current frame to complete.
    // The fence beeing free allow this one to move on.
    if (!vulkan_fence_wait(
            &context,
            &context.in_flight_fences[context.current_frame],
            UINT64_MAX)) {
        LOG_WARN("in flight fence wait failure!");
        return FALSE;
    }

    // acquire next image from the swapchain. pass along the current frame semaphore
    // this semaphore will be triggered when this completes.
    // Same semaphore will be used later on by queue submission to ensure image is avalaible.
    if(!vulkan_swapchain_acquire_next_image_index(
            &context,
            &context.swapchain,
            UINT64_MAX,
            context.image_available_semaphore[context.current_frame],
            0,
            &context.image_index)) {
        return FALSE;
    }

    // retrieve relevant command buffers and start recording commands on it.
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, FALSE, FALSE, FALSE);
    
    // Dynanmic state

    // by default origin point of vulkan is not 0,0 and left bottom corner.
    // it is on top corner. Unlike OpenGL.
    // so tweaking vulkan to make it consistent with OpenGL.
    VkViewport viewport;
    viewport.x = 0;
    viewport.y = (f32)context.framebuffer_height;
    viewport.width  =  (f32)context.framebuffer_width;
    viewport.height = -(f32)context.framebuffer_height;
    viewport.minDepth = 0.0f; // range for image of Z axis
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width  = context.framebuffer_width;
    scissor.extent.height = context.framebuffer_height;

    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    //context.main_renderpass.w = 50;
    //context.main_renderpass.h = 50;

    //Begin the render pass
    vulkan_renderpass_begin(
            command_buffer,
            &context.main_renderpass,
            context.swapchain.framebuffers[context.image_index].handle
        );

    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time){
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // end renderpass
    vulkan_renderpass_end(command_buffer, &context.main_renderpass);

    // stop registering command to vulkan command buffer.
    vulkan_command_buffer_end(command_buffer);

    // make sure previous image is not using this image.
    if(context.image_in_flight[context.image_index] != VK_NULL_HANDLE) {
        vulkan_fence_wait(
                &context,
                context.image_in_flight[context.image_index],
                UINT64_MAX
            );
    }

    // mark the image fence as in use by this fence.
    context.image_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];

    // reset the fence for use on the next frame.
    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);

    // submit the queue and wait for the operation to be complete.
    // begin queue submission
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    // command buffer(s) to be executed
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;

    // the semaphore(s) to be signaled when the queue is complete.
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphore[context.current_frame];

    // wait semaphores ensures that the operation cannot begin until the image is available.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphore[context.current_frame];

    // each semaphore waits on the corresponding stage to complete, 1:1 ratio.
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent colour attachment.
    // writes from executing the semaphores signals (ie one frame is presented at a time).
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(
            context.device.graphics_queue,
            1,
            &submit_info,
            context.in_flight_fences[context.current_frame].handle
        );
    if(result != VK_SUCCESS) {
        LOG_ERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, TRUE));
    }

    vulkan_command_buffer_update_submitted(command_buffer);
    // end queue submission

    // give the image back to the swapchain
    vulkan_swapchain_present(
            &context,
            &context.swapchain,
            context.device.graphics_queue,
            context.device.present_queue,
            context.queue_complete_semaphore[context.current_frame],
            context.image_index
        );

    return TRUE;
}

//??? need to investigate
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_INFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_TRACE(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}

//???
i32 find_memory_index(u32 type_filter, u32 property_flags) {
    VkPhysicalDeviceMemoryProperties memory_properties;    
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);
    for(i32 i = 0; i<memory_properties.memoryTypeCount; i++) {
        if(type_filter & (1 <<i ) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
            return i;
        }
    }
    LOG_WARN("unable to find the sutable memory type!");
    return -1;
}
void create_command_buffers(renderer_backend* backend) {
    // a separate command buffer for every image of the swapchain.

    // if darray not allocated yet, reserving memory.
    if(!context.graphics_command_buffers){
        context.graphics_command_buffers = darray_reserve(
                vulkan_command_buffer,
                context.swapchain.image_count
            );
    }
    for(u32 i = 0; i<context.swapchain.image_count; i++){
        // if commands command buffers not destroyed in the darray, doing so.
        if(context.graphics_command_buffers[i].handle){
            vulkan_command_buffer_free(
                    &context,
                    context.device.graphics_command_pool,
                    &context.graphics_command_buffers[i]
                );
        }
        kzero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        vulkan_command_buffer_allocate(
                &context,
                context.device.graphics_command_pool,
                TRUE,
                &context.graphics_command_buffers[i]
            );
    }
}

void regenerate_frame_buffers(
        renderer_backend* backend,
        vulkan_swapchain* swapchain,
        vulkan_renderpass* renderpass){
    // creating a frame buffer for each swpachain image. 
    for(u32 i = 0 ; i < swapchain->image_count; ++i){
        // make this editable.
        u32 attachment_count = 2;
        VkImageView attachments[] = {
            swapchain->views[i],
            swapchain->depth_attachment.view
        };

        vulkan_framebuffer_create(
                &context,
                renderpass,
                context.framebuffer_width,
                context.framebuffer_height,
                attachment_count,
                attachments,
                &context.swapchain.framebuffers[i]
            );
    }
}

b8 recreate_swapchain(renderer_backend* backend){
    // if already being recreated, do not try again.
    if(context.recreating_swapchain) {
        LOG_DEBUG("recreate_swapchain called when alreaydy recreating. Booting.");
        return FALSE;
    }

    // detect if the window is too small to be drawn.
    if(context.framebuffer_width == 0 || context.framebuffer_height == 0){
        LOG_DEBUG("recreate_swapchain called when window is < 1 in a dimension. Booting.");
        return FALSE;
    }

    // set flag. 
    context.recreating_swapchain = TRUE;

    // wait for logical device to not beeing busy.
    vkDeviceWaitIdle(context.device.logical_device);

    // clear these out just in case
    for (u32 i = 0; i < context.swapchain.image_count; ++i){
        context.image_in_flight[i] = 0;
    }

    // requery support and depth format (in case it changed somehow)
    vulkan_device_query_swapchain_support(
            context.device.physical_device,
            context.surface,
            &context.device.swapchain_support
        );
    vulkan_device_detect_depth_format(&context.device);

    // actually recreating swapchain.
    vulkan_swapchain_recreate(
            &context,
            cached_framebuffer_width,
            cached_framebuffer_height,
            &context.swapchain
        );

    // sync framebuffer size with cached sizes and reset cache
    context.framebuffer_width  = cached_framebuffer_width;
    context.framebuffer_height = cached_framebuffer_height;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    cached_framebuffer_width  = 0;
    cached_framebuffer_height = 0;

    // sync framebuffer size generation.
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

    // cleanup swapchain
    for(u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_command_buffer_free(
                &context,
                context.device.graphics_command_pool,
                &context.graphics_command_buffers[i]
            );
    }

    // cleanup framebuffers
    for(u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_framebuffer_destroy(
                &context,
                &context.swapchain.framebuffers[i]
            );
    }

    // reconfiguring main renderpass render zone.
    context.main_renderpass.x = 0;
    context.main_renderpass.y = 0;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    regenerate_frame_buffers(backend, &context.swapchain, &context.main_renderpass);

    create_command_buffers(backend);

    context.recreating_swapchain = FALSE;
    return TRUE;
}
