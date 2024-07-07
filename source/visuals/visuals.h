#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "../utility/vulkan_resource.h"
#include "../utility/vulkan_memory_allocator_resource.h"
#include "../state/client.h"

#include "view.h"

struct a2b10g10r10 {
    std::uint32_t a : 2, b : 10, g : 10, r : 10;
};

// use array of struct because binding a single buffer range per draw is cheap
// all Vulkan and GL implementations support aligning at 256
// TODO: use minUniformBufferOffsetAlignment
struct alignas(256) parameter {
    glm::mat4 model_view_projection_matrix;
    glm::mat4 model_matrix;
};

struct parameters {
    parameter parameters[256];
};

struct meshes {
    a2b10g10r10 vertices_position[1024];
    std::uint16_t faces_vertices[1024];
};

struct visuals {
    visuals(::client& client, VkInstance instance, VkSurfaceKHR surface);

    void draw(::client& client, VkInstance instance, VkSurfaceKHR surface);

    // TODO: let vma handle memory limits
    std::uint32_t vertex_memory_size = 128 * 1024 * 1024;
    std::uint32_t index_memory_size = 128 * 1024 * 1024;
    std::uint32_t pixel_memory_size = 1024 * 1024 * 1024;

    unique_debug_utils_messenger debug_utils_messenger;

    VkPhysicalDevice physical_device;

    VkPhysicalDeviceMemoryProperties properties;

    uint32_t graphics_queue_family = 0;
    uint32_t present_queue_family = 0;

    unique_device device;

    unique_allocator allocator;

    unique_semaphore swapchain_image_ready_semaphore;

    VkQueue graphics_queue, present_queue;

    unique_shader_module vertex_shader_module, fragment_shader_module;

    unique_descriptor_set_layout descriptor_set_layout;

    unique_pipeline_layout pipeline_layout;

    // TODO: replace pixel_buffer with fixed size staging
    // buffer
    unique_allocation pixel_allocation;
    unique_buffer pixel_buffer;
    struct image {
        unique_allocation allocation;
        unique_image image;
        unique_image_view view;
    };
    std::vector<image> images;
    unique_sampler default_sampler;

    uint32_t view_parameters_offset, user_position_offset;
    struct visual_model {
        uint32_t
            position_offset, normal_offset,
            texture_coordinate_offset, indices_offset,
            images_offset;
    };
    std::vector<visual_model> models;

    unique_allocation parameter_allocation;
    unique_buffer parameter_buffer;
    unique_allocation vertex_allocation;
    unique_buffer vertex_buffer;
    unique_allocation index_allocation;
    unique_buffer index_buffer;

    unique_command_pool command_pool;

    std::unique_ptr<::view> view;
};
