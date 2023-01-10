#include "vulkan_renderpass.h"
#include "core/kmemory.h"

void vulkan_renderpass_create(
    vulkan_context* context,
    vulkan_renderpass* out_renderpass,
    f32 x, f32 y, f32 z, f32 h,
    f32 r, f32 g, f32 b, f32 a,
    f32 depth,
    u32 stencil) {

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // binding this render pass to graphics pipeline
    // TODO: make this configurable.
    u32 attachment_description_count = 2;
    VkAttachmentDescription attachment_description[attachment_description_count];

    VkAttachmentDescription color_attachment;
    color_attachment.format = context->swapchain.image_format.format; //TODO make config.
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // a single sample
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // in vulkan render pass LoadOP: 
    // LOAD -> we keep what was render by other pass on this area
    // (to render above and combine renderpasses / subpasses)
    // CLEAR -> we clear what is on the actual render pass area and start form scratch
    // DONT_CARE -> we do nothing whatever the state of the memory is.
    // same goes for StoreOP
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_description[0] = color_attachment;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.format = context->device.depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // a single sample
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachment_description[1] = depth_attachment;

    // Depth attachment reference
    VkAttachmentReference depth_attachment_reference;
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // TODO: other attachment types: (input, resolve, preserve)

    // Depth stencil data.
    subpass.pDepthStencilAttachment = &depth_attachment_reference;

    // input form a shader
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = 0;

    // used to multisample color attachments.
    subpass.pResolveAttachments = 0;

    // not used in this subpass but reserved for next subpass.
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = 0;

    // render pass dependancies. TODO : make this config
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // not other subpass used as src.
    dependency.dstSubpass = 0; // not dst subpass, only one
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // we write on color channel
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // we want to read write access on what?
    dependency.dependencyFlags = 0;

    // Render pass create.
    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = attachment_description_count;
    render_pass_create_info.pAttachments = attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;
    render_pass_create_info.pNext = 0;
    render_pass_create_info.flags = 0;

    VK_CHECK(vkCreateRenderPass(
        context->device.logical_device,
        &render_pass_create_info,
        context->allocator,
        &out_renderpass->handle
    ),
      "error creating renderpass");

    VkRenderPassCreateInfo create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

}

void vulkan_renderpass_destroy(
        vulkan_context* context,
        vulkan_renderpass* renderpass){

    if(renderpass && renderpass->handle) {
        vkDestroyRenderPass(
                context->device.logical_device,
                renderpass->handle,
                context->allocator
        );
        renderpass->handle = 0;
    }
}

void vulkan_renderpass_begin(
        vulkan_command_buffer* command_buffer,
        vulkan_renderpass* renderpass,
        VkFramebuffer frame_buffer){

    VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};  
    begin_info.renderPass = renderpass->handle;
    begin_info.framebuffer = frame_buffer;
    begin_info.renderArea.offset.x = renderpass->x;
    begin_info.renderArea.offset.y = renderpass->y;
    begin_info.renderArea.extent.width = renderpass->w;
    begin_info.renderArea.extent.height = renderpass->h;

    VkClearValue clear_values[2];
    kzero_memory(clear_values, sizeof(VkClearColorValue) * 2);
    clear_values[0].color.float32[0] = renderpass->r;
    clear_values[0].color.float32[1] = renderpass->g;
    clear_values[0].color.float32[2] = renderpass->b;
    clear_values[0].color.float32[3] = renderpass->a;
    clear_values[1].depthStencil.depth = renderpass->depth;
    clear_values[1].depthStencil.stencil = renderpass->stencil;

    begin_info.clearValueCount = 2;
    begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer->handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = K_COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vulkan_renderpass_end(
        vulkan_command_buffer* command_buffer,
        vulkan_renderpass* renderpass) {
    vkCmdEndRenderPass(command_buffer->handle);
    command_buffer->state = K_COMMAND_BUFFER_STATE_RECORDING;
}
