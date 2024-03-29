#include "vulkan_swapchain.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "defines.h"
#include "vulkan_device.h"
#include "vulkan_image.h"

void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);

void vulkan_swapchain_create(
        vulkan_context* context,
        u32 width,
        u32 height,
        vulkan_swapchain* swapchain) {
    create(context, width, height, swapchain);
}

void vulkan_swapchain_recreate(
        vulkan_context* context,
        u32 width,
        u32 height,
        vulkan_swapchain* swapchain) {
    destroy(context, swapchain);
    create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(
        vulkan_context* context,
        vulkan_swapchain* swapchain) {
    destroy(context, swapchain);
}

b8 vulkan_swapchain_acquire_next_image_index(
        vulkan_context *context,
        vulkan_swapchain* swapchain,
        u64 timeout_ns,
        VkSemaphore image_available_semaphore,
        VkFence fence,
        u32* out_image_index){

    VkResult result = vkAcquireNextImageKHR(
            context->device.logical_device,
            swapchain->handle,
            timeout_ns,
            image_available_semaphore,
            fence,
            out_image_index
        );

    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        // if window resize or something else: then swapchain recreation triggered.
        vulkan_swapchain_recreate(
                context,
                context->framebuffer_width,
                context->framebuffer_height,
                swapchain
            );
        return FALSE;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_FATAL("Failed to acquire swapchain image!");
        return FALSE; 
    }
    return TRUE;
}

void vulkan_swapchain_present(
        vulkan_context *context,
        vulkan_swapchain* swapchain,
        VkQueue graphics_queue, // not used?
        VkQueue present_queue,
        VkSemaphore render_complete_semaphore,
        u32 present_image_index){
    
    // Return the imaga to the swapchain for presentations.
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &present_image_index;
    present_info.pResults = 0;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // if window resize or something else: then swapchain recreation triggered.
        vulkan_swapchain_recreate(
                context,
                context->framebuffer_width,
                context->framebuffer_height,
                swapchain
            );
    } else if (result != VK_SUCCESS) {
        LOG_FATAL("Failed to acquire swapchain image!");
    }
    // update context->current_frame index.
    context->current_frame = (context->current_frame + 1) % swapchain->max_frames_in_flight;
}

void create(
        vulkan_context* context,
        u32 width,
        u32 height,
        vulkan_swapchain* swapchain){
    VkExtent2D swapchain_extent = {width, height};
    // Triple buffering here? (since two (or three?) image in process at same time.)
    swapchain->max_frames_in_flight = 2;

    // Choose a swap surface format.
    b8 found = FALSE;
    for(u32 i = 0; i < context->device.swapchain_support.format_count; i++) {
        VkSurfaceFormatKHR format = context->device.swapchain_support.formats[i];
        // checking if prefered preferred format.
        if  (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
             format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            swapchain->image_format = format;
            found = TRUE;
            break;
        }
    }
    // if preferred format not found, just take the first one and roll this it.
    if (!found) {
        swapchain->image_format = context->device.swapchain_support.formats[0];
    }

    // choosing a presentation mode.
    // FIFO may be laggy for video game if image render happens to be faster than image drawning.
    // MAILBOX_KHR will not be "first in first out" but: present the latest image added to buffer.
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i <context->device.swapchain_support.present_mode_count; i++) {
        VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }

    // requirey sapchain support in case...
    vulkan_device_query_swapchain_support(
        context->device.physical_device,
        context->surface,
        &context->device.swapchain_support);

    // ?????????????????
    // swapchain extent
    if(context->device.swapchain_support.capabilities.currentExtent.width != UINT32_MAX) {
        swapchain_extent = context->device.swapchain_support.capabilities.currentExtent;
    }
    
    VkExtent2D min = context->device.swapchain_support.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchain_support.capabilities.maxImageExtent;
    swapchain_extent.width  = KCLAMP(swapchain_extent.width, min.width, max.width);
    swapchain_extent.height = KCLAMP(swapchain_extent.height, min.height, max.height);

    // triple bufferring? 
    u32 image_count = context->device.swapchain_support.capabilities.minImageCount + 1;

    // caping image_count to maxImageCount
    if(context->device.swapchain_support.capabilities.maxImageCount>0 &&
        image_count> context->device.swapchain_support.capabilities.maxImageCount) {
        image_count = context->device.swapchain_support.capabilities.maxImageCount;
    }

    // Swapchain create info.
    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = context->surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = swapchain->image_format.format;
    swapchain_create_info.imageColorSpace = swapchain->image_format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1; // ???
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup the queue family indices

    // if graphic and present queue are 2 queues: they need to share imges of the swapchain
    // therefore setting the swapchain image sharing mode to concurrent.
    // otherwise if they are on the same queue, exclusive use is ok.
    if (context->device.graphics_queue_index != context->device.present_queue_index) {
        u32 queueFamilyIndices[] = {
            (u32) context->device.graphics_queue_index,
            (u32) context->device.present_queue_index
        };
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }

    // -> transform : orientation mode (portrait landscape).
    swapchain_create_info.preTransform = context->device.swapchain_support.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

    // creating swapchain.
    VK_CHECK(
        vkCreateSwapchainKHR(
                context->device.logical_device,
                &swapchain_create_info,
                context->allocator,
                &swapchain->handle
            ),
        "error creating swapchain"
        );

    context->current_frame = 0; // starrt with a zero frame index.

    // images and views.

    // images: those are created automatically by swapchain.
    // Just need to query them and store them in swapchain struct.
    swapchain->image_count = 0;
    VK_CHECK(
            vkGetSwapchainImagesKHR(
                context->device.logical_device,
                swapchain->handle,
                &swapchain->image_count,
                0
            ),
            "error getting swapchain images."
        );
    if (!swapchain->images) {
        swapchain->images = (VkImage*)kallocate(
                sizeof(VkImage) * swapchain->image_count,
                MEMORY_TAG_RENDERER
            );
    }
    VK_CHECK(
            vkGetSwapchainImagesKHR(
                context->device.logical_device,
                swapchain->handle,
                &swapchain->image_count,
                swapchain->images
            ),
            "error getting swapchain images."
        );

    // views: these needs to be created by us, and then stored in swapchain struct
    if (!swapchain->views) {
        swapchain->views = (VkImageView*)kallocate(
                sizeof(VkImageView) * swapchain->image_count,
                MEMORY_TAG_RENDERER
            );
    }
    for (u32 i = 0; i < swapchain->image_count; i++){
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain->image_format.format;
        // Ensuring usage of colored image 
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        VK_CHECK(
                vkCreateImageView(
                    context->device.logical_device,
                    &view_info,
                    context->allocator,
                    &swapchain->views[i]
                ),
                "error creating image view."
            );
    }

    // unlike images, swapchain does not provides services to create depth buffer.
    // has to be manually created as a VkImage

    // Depth ressources
    if(!vulkan_device_detect_depth_format(&context->device)) {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        LOG_FATAL("Failed to find a supported format.");
    }

    vulkan_image_create(
            context,
            VK_IMAGE_TYPE_2D,
            swapchain_extent.width,
            swapchain_extent.height,
            context->device.depth_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // using GPU memory, not shared memory
            TRUE,
            VK_IMAGE_ASPECT_DEPTH_BIT,          // teling vulkan this image will be used to store depth?
            &swapchain->depth_attachment
        );

    LOG_INFO("Swapchain created successfully");
}

void destroy(vulkan_context* context, vulkan_swapchain* swapchain) {
    vkDeviceWaitIdle(context->device.logical_device);

    // destroy depth buffer
    vulkan_image_destroy(context, &swapchain->depth_attachment);

    // destroy view of image created by swapchain
    // but views only (destruction of images is done by swapchain)
    for(u32 i = 0; i < swapchain->image_count; i++) {
        vkDestroyImageView(
                context->device.logical_device,
                swapchain->views[i],
                context->allocator
            );
    }
    vkDestroySwapchainKHR(
            context->device.logical_device,
            swapchain->handle,
            context->allocator
        );
}
