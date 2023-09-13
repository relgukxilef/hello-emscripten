#include "hello.h"

#include <cstdio>

#include <vulkan/vulkan_core.h>

#include "utility/vulkan_resource.h"

hello::hello(VkInstance instance, VkSurfaceKHR surface) :
    visuals(new ::visuals(client, instance, surface)),
    audio(new ::audio())
{
    std::printf("Servus Welt 3!\n");
}

void hello::draw(VkInstance instance, VkSurfaceKHR surface) {
    try {
        visuals->draw(client, instance, surface);
    } catch (vulkan_error& error) {
        std::fprintf(stderr, "Vulkan error. %s\n", error.what());
        if (error.result == VK_ERROR_DEVICE_LOST) {
            visuals.reset();
            visuals.reset(new ::visuals(client, instance, surface));
        } else {
            throw;
        }
    }
}

void hello::update(input& input) {
    client.update(input);
}
