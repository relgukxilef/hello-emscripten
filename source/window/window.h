#include <memory>
#include <vulkan/vulkan.h>

struct window_internal;

struct window {
    window();
    ~window();

    VkSurfaceKHR get_surface();

    std::unique_ptr<window_internal> internal;
};