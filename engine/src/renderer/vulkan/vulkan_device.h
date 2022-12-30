#pragma once
#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

b8 vulkan_device_create(vulkan_context* context);
void vulkan_device_destroy(vulkan_context* context);

