#pragma once

#include <vulkan/vulkan_core.h>

#include "../utility/vulkan_resource.h"

struct visuals {
    visuals(VkInstance instance, VkSurfaceKHR surface);

    void draw();

    VkInstance instance;
    VkSurfaceKHR surface;

    unique_debug_utils_messenger debug_utils_messenger;

    unique_device device;
    unique_command_pool command_pool;
    unique_swapchain swapchain;
    unique_semaphore render_finished_semaphore;
    unique_fence render_finished_fence;

    VkCommandBuffer draw_command_buffer;
};

