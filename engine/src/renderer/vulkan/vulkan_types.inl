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

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
}vulkan_device;

typedef struct vulkan_context {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    vulkan_device device;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

} vulkan_context;

//TODO: better implementation of this... (by reimplementing assert).
#define VK_CHECK(expr, message)                     \
    {                                   \
    if(expr != VK_SUCCESS) {                    \
        LOG_ERROR("VK process returned with result: %d", expr); \
    }                               \
    KASSERT_MSG(expr == VK_SUCCESS, message);           \
    }

