#include "hello.h"

#include <cstdio>

#include <vulkan/vulkan_core.h>

hello::hello(VkInstance instance, VkSurfaceKHR surface) :
    visuals(instance, surface)
{
    printf("Servus Welt 3!");
}

void hello::draw() {
    visuals.draw();
}
