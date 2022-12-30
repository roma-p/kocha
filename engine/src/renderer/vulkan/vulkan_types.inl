#pragma once
#include "defines.h"
#include "core/logger.h"
#include "core/assert.h"

#include <vulkan/vulkan.h>

typedef struct vulkan_device{
     VkPhysicalDevice physical_device;
     VkDevice logical_device;
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
#define VK_CHECK(expr, message)						\
    {									\
	if(expr != VK_SUCCESS) {					\
	    LOG_ERROR("VK process returned with result: %d", expr);	\
	}								\
	KASSERT_MSG(expr == VK_SUCCESS, message);			\
    }


