#pragma once
#include "defines.h"
#include "core/logger.h"
#include "core/assert.h"

#include <vulkan/vulkan.h>

typedef struct vulkan_swapchain_support_info{
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
}vulkan_swapchain_support_info;

typedef struct vulkan_device{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;

    vulkan_swapchain_support_info swapchain_support;

    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkCommandPool graphics_command_pool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
    
    VkFormat depth_format;

}vulkan_device;

typedef struct vulkan_image {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
} vulkan_image;

// TODO : change this dangerous enum valus to more specific... K_RENDERPASS_READY
typedef enum vulkan_renderpass_state {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED,
}vulkan_renderpass_state;

typedef struct vulkan_renderpass {
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;
    f32 depth;
    u32 stencil;
    vulkan_renderpass_state state;
}vulkan_renderpass;

typedef struct vulkan_framebuffer{
    VkFramebuffer handle;
    u32 attachment_count;
    VkImageView* attachments;
    vulkan_renderpass* renderpass;
}vulkan_framebuffer;

typedef struct vulkan_swapchain {
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkSwapchainKHR handle;
    u32 image_count;
    VkImage* images;
    VkImageView* views;
    vulkan_image depth_attachment;
    // framebuffer ued for on screen rendering.
    vulkan_framebuffer* framebuffers;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
    K_COMMAND_BUFFER_STATE_READY,
    K_COMMAND_BUFFER_STATE_RECORDING,
    K_COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    K_COMMAND_BUFFER_STATE_RECORDING_ENDED,
    K_COMMAND_BUFFER_STATE_SUBMITTED,
    K_COMMAND_BUFFER_STATE_NOT_ALLOCATED,
}vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;

    vulkan_command_buffer_state state;
}vulkan_command_buffer;

typedef struct vulkan_fence {
    VkFence handle;
    b8 is_signaled; // set to True when fence is signaled because operation is completed.
}vulkan_fence;

typedef struct vulkan_context {
    // frame buffer current width and height
    u32 framebuffer_width;
    u32 framebuffer_height;

    // current generation of framebuffer size. if it does not match
    // framebuffer_size_last_generation, then a new one shall be created.
    u64 framebuffer_size_generation;

    // the generation of the framebuffer when it was last created. 
    // Set to framebuffer_size_generation when updated.
    u64 framebuffer_size_last_generation;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
    vulkan_device device;
    vulkan_swapchain swapchain;
    vulkan_renderpass main_renderpass;

    //darray
    vulkan_command_buffer* graphics_command_buffers;

    // nota: semaphores: GPU to GPU // fences GPU to application

    //darray image is done beeing present to screen and ready to render again.
    VkSemaphore* image_available_semaphore;

    //darray image have been run by a queue and rdy to be presented
    VkSemaphore* queue_complete_semaphore;
    // ???
    u32 in_flight_fence_count;
    vulkan_fence* in_flight_fences;

    vulkan_fence** image_in_flight;

    u32 image_index;
    u32 current_frame;

    b8 recreating_swapchain;

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);


} vulkan_context;

//TODO: better implementation of this... (by reimplementing assert).
#define VK_CHECK(expr, message)                     \
    {                                               \
        KASSERT_MSG(expr == VK_SUCCESS, message);   \
    }

