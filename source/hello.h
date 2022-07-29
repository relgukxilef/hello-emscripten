#pragma once

#include <vulkan/vulkan_core.h>

struct hello {
    hello(VkInstance instance, VkSurfaceKHR surface);
    void draw();


};
