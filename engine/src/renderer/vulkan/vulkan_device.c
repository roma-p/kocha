#include "renderer/vulkan/vulkan_device.h"

#include "containers/darray.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "defines.h"

typedef struct vulkan_physical_device_requirements {
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    // darray
    const char** device_extension_names;
    b8 sampler_anisotropy;
    b8 discrete_gpu;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info {
    u32 graphics_family_index;
    u32 present_family_index;
    u32 compute_family_index;
    u32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

b8 select_physical_device(vulkan_context* context);

b8 physical_device_meets_requirements(
    VkPhysicalDevice device, VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_familiy_info,
    vulkan_swapchain_support_info* out_swapchain_support);

b8 vulkan_device_create(vulkan_context* context) {
    if (!select_physical_device(context)) {
        return FALSE;
    }
    LOG_INFO("Creating logical device...");
    b8 present_shares_graphics_queue = context->device.graphics_queue_index == context->device.present_queue_index;
    b8 transfer_shares_graphics_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;
    u32 index_count = 1;

    if(!present_shares_graphics_queue) {
        index_count ++;
    }
    if(!transfer_shares_graphics_queue) {
        index_count ++;
    }

    u32 indices[index_count];
    u8 index = 0;
    indices[index++] = context->device.graphics_queue_index;
    if(!present_shares_graphics_queue) {
        indices[index++] = context->device.present_queue_index;
    }
    if(!transfer_shares_graphics_queue) {
        indices[index++] = context->device.transfer_queue_index;
    }

    VkDeviceQueueCreateInfo queue_create_info[index_count];
    for(u32 i = 0; i < index_count; i++) {
        queue_create_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[i].queueFamilyIndex = indices[i];
        queue_create_info[i].queueCount = 1;
        // if (indices[i] == context->device.graphics_queue_index) {
        //     queue_create_info[i].queueCount = 2;
        // }
        queue_create_info[i].flags = 0;
        queue_create_info[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queue_create_info[i].pQueuePriorities = &queue_priority;
    }

    //TODO: shall be config driven.
    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    b8 portability_required = FALSE;
    u32 available_extension_count = 0;
    VkExtensionProperties* available_extensions = 0;

    VK_CHECK(
            vkEnumerateDeviceExtensionProperties(
                context->device.physical_device,
                0,
                &available_extension_count,
                0
            ),
            "error listing available extensions properties."
        );

    if (available_extension_count != 0) {
        available_extensions = kallocate(
                sizeof(VkExtensionProperties) * available_extension_count,
                MEMORY_TAG_RENDERER
            );
        VK_CHECK(
                vkEnumerateDeviceExtensionProperties(
                    context->device.physical_device,
                    0,
                    &available_extension_count,
                    available_extensions
                ),
                "error listing available extensions properties."
            );

        for (u32 i = 0; i < available_extension_count; ++i) {
            if (strings_equal(available_extensions[i].extensionName,
                              "VK_KHR_portability_subset")) {
                LOG_INFO("Adding required extension 'VK_KHR_portability_subset'.");
                portability_required = TRUE;
                break;
            }
        }
    }

    kfree(
            available_extensions,
            sizeof(VkExtensionProperties) * available_extension_count,
            MEMORY_TAG_RENDERER
        );

    u32 extension_count = portability_required ? 2 : 1;
    const char** extension_names = portability_required
            ? (const char* [2]) { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                 "VK_KHR_portability_subset" }
            : (const char* [1]) { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo device_create_info= {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = extension_count;
    device_create_info.ppEnabledExtensionNames = extension_names;

    // deprecated in new vulkan
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    LOG_TRACE("about to create logical device");

    VK_CHECK(
        vkCreateDevice(
                context->device.physical_device,
                &device_create_info,
                context->allocator,
                &context->device.logical_device
            ),
        "error creating logical device."
        );

    LOG_INFO("Logical device created.");

    vkGetDeviceQueue(
            context->device.logical_device,
            context->device.graphics_queue_index,
            0,
            &context->device.graphics_queue
        );
    vkGetDeviceQueue(
            context->device.logical_device,
            context->device.present_queue_index,
            0,
            &context->device.present_queue
        );
    vkGetDeviceQueue(
            context->device.logical_device,
            context->device.transfer_queue_index,
            0,
            &context->device.transfer_queue
        );
    LOG_INFO("Queues obtained.");

    // Create Command Pool for graphics queue
    VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_create_info.queueFamilyIndex = context->device.graphics_queue_index;
    // allows every single command buffer to be individually reset.
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(
            vkCreateCommandPool(
                context->device.logical_device,
                &pool_create_info,
                context->allocator,
                &context->device.graphics_command_pool
            ),
            "error creating command pool"
        );


    return TRUE;
}

void vulkan_device_destroy(vulkan_context* context) {
    context->device.graphics_queue = 0;
    context->device.transfer_queue = 0;
    context->device.present_queue = 0;

    // destroying command buffer pool
    LOG_INFO("Destroying command buffer pools.");
    vkDestroyCommandPool(
            context->device.logical_device,
            context->device.graphics_command_pool,
            context->allocator
        );

    // destroying logical device
    LOG_INFO("Destroying logical device.");
    if(context->device.logical_device) {
        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = 0;
    }

    // destroying physical device
    LOG_INFO("Releasing physical device ressources...");
    context->device.physical_device = 0;
    if(context->device.swapchain_support.formats) {
        kfree(
                context->device.swapchain_support.formats,
                sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.format_count,
                MEMORY_TAG_RENDERER
            );
        context->device.swapchain_support.formats = 0;
        context->device.swapchain_support.format_count = 0;
    }
    if(context->device.swapchain_support.present_modes) {
        kfree(
                context->device.swapchain_support.present_modes,
                sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.present_mode_count,
                MEMORY_TAG_RENDERER
            );
        context->device.swapchain_support.present_modes = 0;
        context->device.swapchain_support.present_mode_count = 0;
    }
    // ???
    kzero_memory(
            &context->device.swapchain_support.capabilities,
            sizeof(context->device.swapchain_support.capabilities)
        );
    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    context->device.transfer_queue_index = -1;
}

void vulkan_device_query_swapchain_support(
        VkPhysicalDevice physical_device, VkSurfaceKHR surface,
        vulkan_swapchain_support_info* out_support_info) {

    // Surface capabilities.
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device, surface, &out_support_info->capabilities),
    "error retrieveing physical device surface capabilities.");

    // Surface formats.
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, surface, &out_support_info->format_count, 0),
        "error retrieveing physical device surface format.");
    if (out_support_info->format_count != 0) {
        if (!out_support_info->formats) {
            out_support_info->formats = kallocate(
                out_support_info->format_count * sizeof(VkSurfaceFormatKHR),
                MEMORY_TAG_RENDERER);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
                physical_device, surface,
                &out_support_info->format_count,
                out_support_info->formats),
            "error retrieveing physical device surface format.");
        }
    }

    // Present modes
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device,
            surface,
            &out_support_info->present_mode_count,
            0),
        "error retrieving physical device surface present modes");
    if (out_support_info->present_mode_count != 0) {
        if (!out_support_info->present_modes) {
            out_support_info->present_modes = kallocate(
                out_support_info->present_mode_count * sizeof(VkPresentModeKHR),
                MEMORY_TAG_RENDERER);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
                physical_device, surface,
                &out_support_info->present_mode_count,
                out_support_info->present_modes),
            "error retrieving physical device surface present modes");
        }
    }
}

b8 vulkan_device_detect_depth_format(vulkan_device* device) {
    // format candidates
    const u64 candidate_count = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u32 i = 0; i < candidate_count; i++ ){
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(
            device->physical_device,
            candidates[i],
            &properties);
        if((properties.linearTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            return TRUE;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            return TRUE;
        }
    }
    return FALSE;
}

// TODO: maybe find a dirty simple way to get a physical device.
// (without precisely selecting them).
b8 select_physical_device(vulkan_context* context) {
    u32 physical_device_count = 0;
    const char* get_device_err = "Error getting physical devices";

    VK_CHECK(
        vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0),
        get_device_err);
    if (physical_device_count == 0) {
        LOG_FATAL("No devices which support Vulkan found.");
        return FALSE;
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    // FIXME: in platform use an array two, instead of a dynamic array?
    VK_CHECK(vkEnumeratePhysicalDevices(
            context->instance, &physical_device_count,
            physical_devices),
        get_device_err);

    for (u32 i = 0; i < physical_device_count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        vulkan_physical_device_requirements requirements = {};
        requirements.graphics = TRUE;
        requirements.present = TRUE;
        requirements.transfer = TRUE;
        requirements.sampler_anisotropy = TRUE;

#if KPLATFORM_APPLE
        requirements.discrete_gpu = FALSE;
#else
        requirements.discrete_gpu = TRUE;
#endif
        requirements.device_extension_names = darray_create(const char*);
        darray_push(
            requirements.device_extension_names,
            &VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        vulkan_physical_device_queue_family_info queue_info = {};

        b8 result = physical_device_meets_requirements(
            physical_devices[i],
            context->surface,
            &properties,
            &features,
            &requirements,
            &queue_info,
            &context->device.swapchain_support);

        if(result) {
            LOG_INFO("Selected deivce: '%s.'.", properties.deviceName);
            switch(properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    LOG_INFO("GPU type is unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    LOG_INFO("GPU type is integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    LOG_INFO("GPU type is descrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    LOG_INFO("GPU type is virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    LOG_INFO("GPU type is cpu.");
                    break;
            }
            LOG_INFO("GPU driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));
            LOG_INFO("Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            for(u32 j=0; j<memory.memoryHeapCount; ++j) {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    LOG_INFO("local GPU memory: %.2f Gib", memory_size_gib);
                } else {
                    LOG_INFO("shared System memory: %.2f Gib", memory_size_gib);
                }
            }

            context->device.physical_device = physical_devices[i];
            context->device.graphics_queue_index = queue_info.graphics_family_index;
            context->device.present_queue_index = queue_info.present_family_index;
            context->device.transfer_queue_index = queue_info.transfer_family_index;

            context->device.properties = properties;
            context->device.features = features;
            context->device.memory = memory;
            break;
        }
    }
  
    if(!context->device.physical_device) {
        LOG_ERROR("No physical devices were found which meet the requirements.");
        return FALSE;
    }

    LOG_INFO("Physical device selected.");
    return TRUE;
}

b8 physical_device_meets_requirements(
        VkPhysicalDevice device, VkSurfaceKHR surface,
        const VkPhysicalDeviceProperties* properties,
        const VkPhysicalDeviceFeatures* features,
        const vulkan_physical_device_requirements* requirements,
        vulkan_physical_device_queue_family_info* out_queue_familiy_info,
        vulkan_swapchain_support_info* out_swapchain_support) {

    out_queue_familiy_info->compute_family_index = -1;
    out_queue_familiy_info->present_family_index = -1;
    out_queue_familiy_info->graphics_family_index = -1;
    out_queue_familiy_info->transfer_family_index = -1;

    // check GPU
    if (requirements->discrete_gpu) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            LOG_INFO("Device not a discrete GPU as required, skipping");
            return FALSE;
        }
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queue_family_count,
        queue_families);

    LOG_INFO("Graphics | Present | Compute | Transfer | Name")

    u8 min_transfer_code = 255;
    for (u32 i = 0; i < queue_family_count; i++) {
        u8 current_transfer_code = 0;

        // check graphics queue
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            out_queue_familiy_info->graphics_family_index = i;
            ++current_transfer_code;
        }

        // check compute queue
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            out_queue_familiy_info->compute_family_index = i;
            ++current_transfer_code;
        }

        // check transfer queue
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            // if queue has transfer bit and has minimum queue:
            // probably dedicated transfer queue.
            if (current_transfer_code <= min_transfer_code) {
                min_transfer_code = current_transfer_code;
                out_queue_familiy_info->transfer_family_index = i;
            }
        }

        // check present queue
        VkBool32 supports_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(
                device, i, surface,
                &supports_present),
            "error queuring device support queue");
        if (supports_present) {
            out_queue_familiy_info->present_family_index = i;
      
        }
    }
    LOG_INFO("       %d |       %d |       %d |       %d  | %s",
        out_queue_familiy_info->graphics_family_index !=-1,
        out_queue_familiy_info->present_family_index != -1,
        out_queue_familiy_info->compute_family_index != -1,
        out_queue_familiy_info->transfer_family_index != -1,
        properties->deviceName);

    if (
            (!requirements->graphics || (requirements->graphics && out_queue_familiy_info->graphics_family_index != -1)) &&
            (!requirements->present  || (requirements->present  && out_queue_familiy_info->present_family_index  != -1)) &&
            (!requirements->compute  || (requirements->compute  && out_queue_familiy_info->compute_family_index  != -1)) &&
            (!requirements->transfer || (requirements->transfer && out_queue_familiy_info->transfer_family_index != -1))) {
        LOG_INFO("Device meets queue requirements");
        LOG_TRACE("Graphics Family Index: %i", out_queue_familiy_info->graphics_family_index);
        LOG_TRACE("Present Family Index:  %i", out_queue_familiy_info->present_family_index);
        LOG_TRACE("Transfer Family Index: %i", out_queue_familiy_info->transfer_family_index);
        LOG_TRACE("Compute Family Index:  %i", out_queue_familiy_info->compute_family_index);
        // query swapchain support
        vulkan_device_query_swapchain_support(device, surface, out_swapchain_support);

        if(out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count <1 ){
            if(out_swapchain_support->formats) {
                kfree(
                    out_swapchain_support->formats,
                    sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count,
                    MEMORY_TAG_RENDERER);
            }
            if(out_swapchain_support->present_modes) {
                kfree(
                    out_swapchain_support->present_modes,
                    sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count,
                    MEMORY_TAG_RENDERER);
            }
            LOG_INFO("required swapchain not present, skipping.");
            return FALSE;
        }
    
            // Device extensions
            if (requirements->device_extension_names) {
    
                u32 available_extension_count = 0;
                VkExtensionProperties* available_extensions = 0;
                VK_CHECK(vkEnumerateDeviceExtensionProperties(
                        device, 0,
                        &available_extension_count, 0),
                    "error retreiving device extension properties");
                if(available_extension_count != 0) {
                        available_extensions = kallocate(
                            sizeof(VkExtensionProperties) * available_extension_count,
                            MEMORY_TAG_RENDERER);
                        VK_CHECK(vkEnumerateDeviceExtensionProperties(
                                device, 0,
                                &available_extension_count,
                                available_extensions),
                            "error retreiving device extension properties");
    
                    u32 required_extension_count = darray_length(requirements->device_extension_names);
                    for(u32 i = 0; i<required_extension_count; i++) {
                        b8 found = FALSE;
                        for(u32 j; j <available_extension_count; j++) {
                            if(strings_equal(requirements->device_extension_names[i], available_extensions[j].extensionName)) {
                                found = TRUE;
                                break;
                            }
                        }
                        if(!found) {
                            LOG_INFO("Required extension not found: '%s'. Skipping device",
                            requirements->device_extension_names[i]);
                            kfree(
                                available_extensions,
                                sizeof(VkExtensionProperties) * available_extension_count,
                                MEMORY_TAG_RENDERER);
                        return FALSE;
                        }
                    }
                    kfree(
                        available_extensions,
                        sizeof(VkExtensionProperties) * available_extension_count,
                        MEMORY_TAG_RENDERER);
                }
            }
    
            // Sampler anisostropy
            if (requirements->sampler_anisotropy &&  ! features->samplerAnisotropy) {
                LOG_INFO("Device does not support sampler anisotropy, skipping");
                return FALSE;
        }

    // device meets all requirements
    return TRUE;
    }
  return FALSE;
}

