#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "../utility/vulkan_resource.h"

struct image {
    VkCommandBuffer draw_command_buffer;
    unique_fence draw_finished_fence;
    unique_semaphore draw_finished_semaphore;
};

struct view {
    view(struct visuals& v);

    VkResult draw(struct visuals& v);

    unique_command_pool command_pool;
    unique_swapchain swapchain;

    std::unique_ptr<image[]> images;
};

