#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "../utility/vulkan_resource.h"

struct image {
    VkCommandBuffer draw_command_buffer;

    VkDescriptorSet descriptor_set;

    unique_image_view image_view;
    unique_framebuffer framebuffer;

    unique_semaphore draw_finished_semaphore;
    unique_fence draw_finished_fence;
};

struct view {
    view(struct visuals& v, VkInstance instance, VkSurfaceKHR surface);

    VkResult draw(struct visuals& v);

    unique_descriptor_pool descriptor_pool;

    unique_command_pool command_pool;
    unique_swapchain swapchain;

    VkExtent2D surface_extent;

    unique_render_pass render_pass;

    unique_pipeline pipeline;

    std::unique_ptr<image[]> images;
};

