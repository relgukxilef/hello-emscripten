#pragma once

#include <vulkan/vulkan_core.h>

#include "visuals/visuals.h"

struct hello {
    hello(VkInstance instance, VkSurfaceKHR surface);
    void draw(VkInstance instance, VkSurfaceKHR surface);

    std::unique_ptr<visuals> visuals;
};
