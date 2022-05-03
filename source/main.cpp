#include <iostream>
#include <cstring>
#include <vulkan/vulkan_core.h>

#include "utility/window/window.h"
#include "utility/vulkan_resource.h"
#include "utility/out_ptr.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*
) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << "validation layer error: " << callback_data->pMessage <<
            std::endl;
        // ignore error caused by Nsight
        if (strcmp(callback_data->pMessageIdName, "Loader Message") != 0)
            throw std::runtime_error("vulkan error");
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cout << "validation layer warning: " << callback_data->pMessage <<
            std::endl;
    }

    return VK_FALSE;
}

int main() {
    window w;

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
            w.get_instance(), "vkCreateDebugUtilsMessengerEXT"
    );
    unique_debug_utils_messenger debug_utils_messenger;
    {
        check(vkCreateDebugUtilsMessengerEXT(
            w.get_instance(), &debug_utils_messenger_create_info, nullptr,
            out_ptr(debug_utils_messenger)
        ));
    }

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

    unique_device device;

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
            .queueCreateInfoCount = std::size(queue_create_infos),
            .pQueueCreateInfos = queue_create_infos,
            .enabledExtensionCount = std::size(enabled_extension_names),
            .ppEnabledExtensionNames = enabled_extension_names,
            .pEnabledFeatures = &device_features
        };

        check(vkCreateDevice(
            physical_device, &create_info, nullptr, out_ptr(device)
        ));
    }
    current_device = device.get();

    VkQueue graphics_queue, present_queue;

    // retreive queues
    vkGetDeviceQueue(device.get(), graphics_queue_family, 0, &graphics_queue);
    vkGetDeviceQueue(device.get(), present_queue_family, 0, &present_queue);

    // create swap chains
    uint32_t format_count = 0, present_mode_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &format_count, nullptr
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &present_mode_count, nullptr
    );
    if (format_count == 0) {
        throw std::runtime_error("no surface formats supported");
    }
    if (present_mode_count == 0) {
        throw std::runtime_error("no surface present modes supported");
    }
    auto formats = std::make_unique<VkSurfaceFormatKHR[]>(format_count);
    auto present_modes =
        std::make_unique<VkPresentModeKHR[]>(present_mode_count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &format_count, formats.get()
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &present_mode_count, present_modes.get()
    );

    VkSurfaceFormatKHR surface_format;

    surface_format = formats[0];
    for (auto i = 0u; i < format_count; i++) {
        auto format = formats[i];
        if (
            format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            surface_format = format;
        }
    }

    unique_command_pool command_pool;

    {
        VkCommandPoolCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = graphics_queue_family,
        };
        check(vkCreateCommandPool(
            device.get(), &create_info, nullptr, out_ptr(command_pool)
        ));
    }

    // view-specific resources
    VkSurfaceCapabilitiesKHR capabilities;
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device, surface, &capabilities
    ));

    unsigned width = capabilities.currentExtent.width;
    unsigned height = capabilities.currentExtent.height;

    VkExtent2D extent = {
        std::max(
            std::min<uint32_t>(width, capabilities.maxImageExtent.width),
            capabilities.minImageExtent.width
        ),
        std::max(
            std::min<uint32_t>(height, capabilities.maxImageExtent.height),
            capabilities.minImageExtent.height
        )
    };

    unique_swapchain swapchain;
    {
        uint32_t queue_family_indices[]{
            graphics_queue_family, present_queue_family
        };
        VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = capabilities.minImageCount,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = std::size(queue_family_indices),
            .pQueueFamilyIndices = queue_family_indices,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            // fifo has the widest support
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };
        check(vkCreateSwapchainKHR(
            device.get(), &create_info, nullptr, out_ptr(swapchain)
        ));
    }

    uint32_t image_count;
    check(vkGetSwapchainImagesKHR(
        device.get(), swapchain.get(), &image_count, nullptr
    ));

    auto swapchain_images = std::make_unique<VkImage[]>(image_count);

    check(vkGetSwapchainImagesKHR(
        device.get(), swapchain.get(), &image_count, swapchain_images.get()
    ));

    // image-specific resources
    unique_semaphore render_finished_semaphore;
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    check(vkCreateSemaphore(
        device.get(), &semaphore_info, nullptr,
        out_ptr(render_finished_semaphore)
    ));

    unique_fence render_finished_fence;
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    check(vkCreateFence(
        device.get(), &fence_info, nullptr, out_ptr(render_finished_fence)
    ));

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool.get(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    check(vkAllocateCommandBuffers(
        device.get(), &command_buffer_info, &command_buffer
    ));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    check(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_images[0],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr, 0, nullptr, 1, &barrier
    );

    check(vkEndCommandBuffer(command_buffer));

    std::cout << "Servus Welt 3!" << std::endl;
    return 0;
}
