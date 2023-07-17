#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "../utility/vulkan_resource.h"
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
    glm::vec4 position;
};

struct parameters {
    parameter parameters[32];
};

struct meshes {
    a2b10g10r10 vertices_position[1024];
    std::uint16_t faces_vertices[1024];
};

struct visuals {
    visuals(::client& client, VkInstance instance, VkSurfaceKHR surface);

    void draw(::client& client, VkInstance instance, VkSurfaceKHR surface);

    std::uint32_t memory_size = 4 * 1024 * 1024;

    unique_debug_utils_messenger debug_utils_messenger;

    VkPhysicalDevice physical_device;

    uint32_t graphics_queue_family = 0;
    uint32_t present_queue_family = 0;

    unique_device device;

    unique_semaphore swapchain_image_ready_semaphore;

    VkQueue graphics_queue, present_queue;

    unique_shader_module vertex_shader_module, fragment_shader_module;

    unique_descriptor_set_layout descriptor_set_layout;

    unique_pipeline_layout pipeline_layout;

    std::uint32_t host_visible_memory_type_index;

    unique_device_memory host_visible_memory, device_local_memory;

    unique_buffer host_visible_buffer, device_local_buffer;

    std::uint32_t view_parameters_offset, user_position_offset;

    std::unique_ptr<::view> view;
};

