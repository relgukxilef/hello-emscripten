#pragma once

#include <vulkan/vulkan_core.h>

#include "visuals/visuals.h"
#include "state/client.h"
#include "state/input.h"

struct hello {
    hello(VkInstance instance, VkSurfaceKHR surface);

    void draw(VkInstance instance, VkSurfaceKHR surface);
    void update(::input& input);

    ::client client;
    std::unique_ptr<::visuals> visuals;
};
