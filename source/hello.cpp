#include "hello.h"

#include <cstdio>

#include <vulkan/vulkan_core.h>

#include "utility/vulkan_resource.h"

hello::hello(VkInstance instance, VkSurfaceKHR surface) :
    visuals(new ::visuals(instance, surface))
{
    printf("Servus Welt 3!");
}

void hello::draw(VkInstance instance, VkSurfaceKHR surface) {
    try {
        visuals->draw();
    } catch (vulkan_error& error) {
        if (error.result == VK_ERROR_DEVICE_LOST) {
            visuals.reset();
            visuals.reset(new ::visuals(instance, surface));
        } else {
            throw;
        }
    }
}
