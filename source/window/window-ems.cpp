#include "window.h"
#include <vulkan/vulkan_core.h>

struct window_internal {
};

window::window() {
    internal = std::make_unique<window_internal>();
}

window::~window() {
}

VkSurfaceKHR window::get_surface() {
    return {};
}