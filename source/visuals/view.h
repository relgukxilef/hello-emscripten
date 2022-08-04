#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "../utility/vulkan_resource.h"

struct image {
    VkCommandBuffer draw_command_buffer;
    unique_fence draw_finished_fence;
    unique_semaphore draw_finished_semaphore;
    unique_image_view image_view;
    unique_framebuffer framebuffer;
};

struct view {
    view(struct visuals& v, VkInstance instance, VkSurfaceKHR surface);

    VkResult draw(struct visuals& v);

    unique_command_pool command_pool;
    unique_swapchain swapchain;

    VkExtent2D surface_extent;

    unique_render_pass render_pass;

    std::unique_ptr<image[]> images;
};

