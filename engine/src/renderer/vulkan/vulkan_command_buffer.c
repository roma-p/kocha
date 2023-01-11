#include "vulkan_command_buffer.h"
#include "core/kmemory.h"
#include "vulkan/vulkan_core.h"


void vulkan_command_buffer_allocate(
        vulkan_context* context,
        VkCommandPool pool,
        b8 is_primary,
        vulkan_command_buffer* out_command_buffer){
    
    // ?
    kzero_memory(out_command_buffer, sizeof(vulkan_command_buffer));

    VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    
    allocate_info.commandPool = pool;

    // only primary can be used to pas ocmmand to GPU by itself.
    // secondary needs to be passed to primary.
    allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY: VK_COMMAND_BUFFER_LEVEL_SECONDARY;

    allocate_info.commandBufferCount = 1; // only allocating ONE command buffer
    allocate_info.pNext = 0;
    out_command_buffer->state = K_COMMAND_BUFFER_STATE_NOT_ALLOCATED;

    VK_CHECK(
            vkAllocateCommandBuffers(
                context->device.logical_device,
                &allocate_info,
                &out_command_buffer->handle
            ),
            "error allocating command buffer"
        );

    out_command_buffer->state = K_COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_free(
        vulkan_context* context,
        VkCommandPool pool,
        vulkan_command_buffer* command_buffer){
    
    vkFreeCommandBuffers(
            context->device.logical_device,
            pool,
            1,
            &command_buffer->handle
        );

    command_buffer->handle = 0;
    command_buffer->state  = K_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vulkan_command_buffer_begin(
        vulkan_command_buffer* command_buffer,
        b8 is_single_use,
        b8 is_renderpass_continue,
        b8 is_simultaneous_use){
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    begin_info.flags = 0; 
    if (is_single_use) {
        // command buffer can only be submitted once.
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (is_renderpass_continue) {
        // ???
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (is_simultaneous_use) {
        // can be rebsumit to different queue
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VK_CHECK(
            vkBeginCommandBuffer(
                command_buffer->handle,
                &begin_info
            ),
            "error while beginning recording on command buffer"
        );
    command_buffer->state = K_COMMAND_BUFFER_STATE_RECORDING;
}

void vulkan_command_buffer_end(
        vulkan_command_buffer* command_buffer){
    VK_CHECK(
            vkEndCommandBuffer(command_buffer->handle),
            "error ending recoring on command buffer"
        );
    command_buffer->state = K_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vulkan_command_buffer_update_submitted(
        vulkan_command_buffer* command_buffer){
    command_buffer->state = K_COMMAND_BUFFER_STATE_SUBMITTED;
}

void vulkan_command_buffer_reset(
        vulkan_command_buffer* command_buffer){
    command_buffer->state = K_COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_allocate_and_begin_single_use(
        vulkan_context* context,
        VkCommandPool pool,
        vulkan_command_buffer* out_command_buffer){
    vulkan_command_buffer_allocate(context, pool, TRUE, out_command_buffer);
    vulkan_command_buffer_begin(out_command_buffer, TRUE, FALSE, FALSE);
}

void vulkan_command_buffer_allocate_and_end_single_use(
        vulkan_context* context,
        VkCommandPool pool,
        vulkan_command_buffer* command_buffer,
        VkQueue queue){
    
    // end recording of command.
    vulkan_command_buffer_end(command_buffer);

    // submit the queue.
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;
    VK_CHECK(
            vkQueueSubmit(queue, 1, &submit_info, 0),
            "erreur submitting command buffer to queue."
        );

    // !!! BLOCKING FUNCTION
    // wait for finish 
    VK_CHECK(
            vkQueueWaitIdle(queue),
            "error waiting for queue iddle"
        );

    // free command buffer
    vulkan_command_buffer_free(context, pool, command_buffer);
}
