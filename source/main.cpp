#include <iostream>
#include <vulkan/vulkan_core.h>

#include "utility/window/window.h"
#include "utility/vulkan_resource.h"
#include "utility/out_ptr.h"

int main() {
    window w;

    VkSurfaceKHR surface(w.get_surface());

    VkPhysicalDevice physical_device;

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(w.get_instance(), &device_count, nullptr);
    {
        auto devices = std::make_unique<VkPhysicalDevice[]>(device_count);
        check(vkEnumeratePhysicalDevices(
            w.get_instance(), &device_count, devices.get()
        ));
        
        physical_device = devices[0]; // just pick the first one
    }

    uint32_t graphics_queue_family = 0;
    uint32_t present_queue_family = 0;
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, nullptr
    );
    auto queue_families =
        std::make_unique<VkQueueFamilyProperties[]>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, queue_families.get()
    );

    for (auto i = 0u; i < queue_family_count; i++) {
        const auto& queueFamily = queue_families[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            physical_device, i, surface, &present_support
        );
        if (present_support) {
            present_queue_family = i;
        }
    }
    if (graphics_queue_family == -1u) {
        throw std::runtime_error("no suitable queue found");
    }

    std::cout << "Servus Welt 2!" << std::endl;
    return 0;
}
