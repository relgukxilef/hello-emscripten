#include "visuals.h"

#include <cstring>
#include <memory>
#include <cstdio>
#include <algorithm>
#include <array>
#include <vector>

#include "../utility/out_ptr.h"

std::vector<uint8_t> read_file(const char* name) {
    FILE* file;
    fopen_s(&file, name, "rb");
    fseek(file, 0, SEEK_END);
    auto size = ftell(file);
    std::vector<uint8_t> content(size);
    fseek(file, 0, SEEK_SET);
    fread(content.data(), sizeof(uint8_t), size, file);
    return content;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*
) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::fprintf(
            stderr, "Validation layer error: %s\n", callback_data->pMessage
        );
        // ignore error caused by Nsight
        if (strcmp(callback_data->pMessageIdName, "Loader Message") != 0)
            throw std::runtime_error("vulkan error");
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::fprintf(
            stderr, "Validation layer warning: %s\n", callback_data->pMessage
        );
    }

    return VK_FALSE;
}

visuals::visuals(VkInstance instance, VkSurfaceKHR surface) {
    // create debug utils messenger
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = nullptr
    };
    auto vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT"
    );
    {
        check(vkCreateDebugUtilsMessengerEXT(
            instance, &debug_utils_messenger_create_info, nullptr,
            out_ptr(debug_utils_messenger)
        ));
    }

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    {
        auto devices = std::make_unique<VkPhysicalDevice[]>(device_count);
        check(vkEnumeratePhysicalDevices(
            instance, &device_count, devices.get()
        ));

        physical_device = devices[0]; // just pick the first one
    }

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
    if (graphics_queue_family == ~0u) {
        throw std::runtime_error("no suitable queue found");
    }

    // create logical device
    {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_infos[]{
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphics_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }, {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = present_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }
        };

        const char* enabled_extension_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceFeatures device_features{};
        VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount =
                static_cast<uint32_t>(std::size(queue_create_infos)),
            .pQueueCreateInfos = queue_create_infos,
            .enabledExtensionCount =
                static_cast<uint32_t>(std::size(enabled_extension_names)),
            .ppEnabledExtensionNames = enabled_extension_names,
            .pEnabledFeatures = &device_features
        };

        check(vkCreateDevice(
            physical_device, &create_info, nullptr, out_ptr(device)
        ));
    }
    current_device = device.get();

    // retreive queues
    vkGetDeviceQueue(device.get(), graphics_queue_family, 0, &graphics_queue);
    vkGetDeviceQueue(device.get(), present_queue_family, 0, &present_queue);

    {
        VkSemaphoreCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        check(vkCreateSemaphore(
            device.get(), &create_info, nullptr,
            out_ptr(swapchain_image_ready_semaphore)
        ));
    }

    // create shader modules
    {
        auto code = read_file("visuals/shaders/solid_vertex.glsl.spv");
        VkShaderModuleCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = (size_t)(code.size()),
            .pCode = reinterpret_cast<uint32_t*>(code.data()),
        };
        check(vkCreateShaderModule(
            device.get(), &create_info, nullptr, out_ptr(vertex_shader_module)
        ));
    }
    {
        auto code = read_file("visuals/shaders/solid_fragment.glsl.spv");
        VkShaderModuleCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = (size_t)(code.size()),
            .pCode = reinterpret_cast<uint32_t*>(code.data()),
        };
        check(vkCreateShaderModule(
            device.get(), &create_info, nullptr, out_ptr(fragment_shader_module)
        ));
    }

    // create pipeline layouts
    VkPipelineLayoutCreateInfo layout_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    check(vkCreatePipelineLayout(
        device.get(), &layout_create_info, nullptr, out_ptr(pipeline_layout)
    ));

    view.reset(new ::view(*this, instance, surface));
}

void visuals::draw(VkInstance instance, VkSurfaceKHR surface) {
    if (view) {
        if (view->draw(*this) != VK_SUCCESS) {
            view.reset(); // delete first
            view.reset(new ::view(*this, instance, surface));
        }
    }
}
