#pragma once

#include <memory>
#include <vulkan/vulkan.h>

struct window_internal;

// TODO: can this be removed?
struct window {
    window();
    ~window();

    VkInstance get_instance();
    VkSurfaceKHR get_surface();

    std::unique_ptr<window_internal> internal;
};
