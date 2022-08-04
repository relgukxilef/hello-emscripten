#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "../utility/vulkan_resource.h"

#include "view.h"

struct visuals {
    visuals(VkInstance instance, VkSurfaceKHR surface);

    void draw(VkInstance instance, VkSurfaceKHR surface);

    unique_debug_utils_messenger debug_utils_messenger;

    VkPhysicalDevice physical_device;

    uint32_t graphics_queue_family = 0;
    uint32_t present_queue_family = 0;

    unique_device device;

    unique_semaphore swapchain_image_ready_semaphore;

    VkQueue graphics_queue, present_queue;

    std::unique_ptr<view> view;
};

