#pragma once

#include <memory>
#include <atomic>

#include <vulkan/vulkan_core.h>

#include "../utility/vulkan_resource.h"
#include "../state/client.h"

struct image {
    // no need to double buffer the command buffer as images are already
    // buffered
    VkCommandBuffer draw_command_buffer;

    std::unique_ptr<VkDescriptorSet[]> descriptor_sets;

    unique_image color_image;
    unique_image depth_image;
    unique_device_memory color_memory;
    unique_device_memory depth_memory;
    unique_image_view color_view;
    unique_image_view depth_view;
    unique_image_view image_view;
    unique_framebuffer framebuffer;

    unique_semaphore draw_finished_semaphore;
    unique_fence draw_finished_fence;

    // to track whether to update the command buffer
    unsigned update_number = 0;
};

struct view {
    view(
        client& c, struct visuals& v, VkInstance instance, VkSurfaceKHR surface
    );

    VkResult draw(struct visuals& v, ::client& client);

    VkSurfaceCapabilitiesKHR capabilities;

    unique_descriptor_pool descriptor_pool;

    unique_swapchain swapchain;

    VkExtent2D surface_extent;

    unsigned descriptor_set_count = 256; // per image

    unique_render_pass render_pass;

    unique_pipeline pipeline;

    std::unique_ptr<image[]> images;

    std::atomic_bool
        command_buffer_recording_begin_fence,
        command_buffer_recording_end_fence;
};
