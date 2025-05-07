#include "utility/trace.h"
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "main-glfw.h"
#include "hello.h"
#include "visuals/visuals.h"
#include "utility/resource.h"
#include "utility/vulkan_resource.h"
#include "utility/xr_resource.h"
#include "utility/out_ptr.h"

struct glfw_error : public std::exception {
    glfw_error() noexcept {};

    const char *what() const noexcept override {
        return "Other glfw error.";
    };
};

struct unique_glfw {
    unique_glfw() {
        if (!glfwInit())
            throw glfw_error();
    }
    ~unique_glfw() { glfwTerminate(); }

    unique_glfw(const unique_glfw&) = delete;
};

GLFWwindow* check(GLFWwindow* w) {
    if (w == nullptr)
        throw glfw_error();
    return w;
}

void glfw_delete_window(GLFWwindow** window) {
    glfwDestroyWindow(*window);
}

using unique_window = unique_resource<GLFWwindow*, glfw_delete_window>;

void error_callback(int error, const char* description) {
    std::fprintf(stderr, "Error %i: %s\n", error, description);
}

struct vk_glfw_visuals {
    vk_glfw_visuals(GLFWwindow* window, ::client& client);
    unique_xr_instance xr_instance;
    unique_instance vk_instance;
    VkPhysicalDeviceMemoryProperties properties;
    uint32_t graphics_queue_family = ~0u;
    uint32_t present_queue_family = ~0u;
    unique_surface surface;
    unique_device vk_device;
    unique_xr_session xr_session;
    unique_xr_swapchain color_swapchain, depth_swapchain;
    std::unique_ptr<::visuals> visuals;
};

vk_glfw_visuals::vk_glfw_visuals(GLFWwindow* window, ::client& client) {
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    const char* required_extensions[]{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    auto extension_count =
        std::size(required_extensions) + glfw_extension_count;
    auto extensions = std::make_unique<const char*[]>(extension_count);
    std::copy(
        glfw_extensions, glfw_extensions + glfw_extension_count,
        extensions.get()
    );
    std::copy(
        required_extensions,
        required_extensions + std::size(required_extensions),
        extensions.get() + glfw_extension_count
    );

    VkApplicationInfo application_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .applicationVersion = 0,
        .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };
    VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extension_count),
        .ppEnabledExtensionNames = extensions.get(),
    };
    VkPhysicalDevice physical_device;

    const char* xr_extensions[]{
        XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
    };
    // TODO: check xrEnumerateInstanceExtensionProperties
    XrInstanceCreateInfo xr_create_info{
        .type = XR_TYPE_INSTANCE_CREATE_INFO,
        .next = nullptr,
        .applicationInfo = {
            .applicationName = "HelloVR",
            .applicationVersion = 1,
            .engineName = "HelloVR",
            .engineVersion = 1,
            .apiVersion = XR_API_VERSION_1_0,
        },
        .enabledExtensionCount = std::size(xr_extensions),
        .enabledExtensionNames = xr_extensions,
    };
    XrSystemGetInfo system_get_info {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .next = nullptr,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };
    XrSystemId system_id;

    try {
        check(xrCreateInstance(
            &xr_create_info, out_ptr(xr_instance)
        ));
        XrInstanceProperties instance_properties {
            .type = XR_TYPE_INSTANCE_PROPERTIES,
        };
        check(xrGetInstanceProperties(
            xr_instance.get(), &instance_properties
        ));
        check(xrGetSystem(xr_instance.get(), &system_get_info, &system_id));

        XrGraphicsRequirementsVulkanKHR requirements {
            .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR,
        };
        PFN_xrGetVulkanGraphicsRequirementsKHR 
        xrGetVulkanGraphicsRequirements2KHR;
        check(xrGetInstanceProcAddr(
            xr_instance.get(), "xrGetVulkanGraphicsRequirements2KHR",
            (PFN_xrVoidFunction*)&xrGetVulkanGraphicsRequirements2KHR
        ));
        check(xrGetVulkanGraphicsRequirements2KHR(
            xr_instance.get(), system_id, &requirements
        ));

        XrVersion vulkan_api_version = XR_MAKE_VERSION(
            VK_API_VERSION_MAJOR(application_info.apiVersion),
            VK_API_VERSION_MINOR(application_info.apiVersion),
            0
        );
        if (
            requirements.minApiVersionSupported > vulkan_api_version ||
            requirements.maxApiVersionSupported < vulkan_api_version
        ) {
            throw std::runtime_error("Vulkan version not supported.");
        }

        XrVulkanInstanceCreateInfoKHR xr_vulkan_create_info{
            .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
            .next = nullptr,
            .systemId = system_id,
            .pfnGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vulkanCreateInfo = &create_info,
        };
        VkResult vk_result;
        PFN_xrCreateVulkanInstanceKHR xrCreateVulkanInstanceKHR;
        check(xrGetInstanceProcAddr(
            xr_instance.get(), "xrCreateVulkanInstanceKHR",
            (PFN_xrVoidFunction*)&xrCreateVulkanInstanceKHR
        ));
        check(xrCreateVulkanInstanceKHR(
            xr_instance.get(), &xr_vulkan_create_info, out_ptr(vk_instance), 
            &vk_result
        ));
        check(vk_result);

        XrVulkanGraphicsDeviceGetInfoKHR get_info {
            .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
            .next = nullptr,
            .systemId = system_id,
            .vulkanInstance = vk_instance.get(),
        };
        PFN_xrGetVulkanGraphicsDevice2KHR xrGetVulkanGraphicsDevice2KHR;
        check(xrGetInstanceProcAddr(
            xr_instance.get(), "xrGetVulkanGraphicsDevice2KHR",
            (PFN_xrVoidFunction*)&xrGetVulkanGraphicsDevice2KHR
        ));
        check(xrGetVulkanGraphicsDevice2KHR(
            xr_instance.get(), &get_info, &physical_device
        ));

    } catch (std::exception& e) {
        std::printf("Starting without VR support. (%s)\n", e.what());
        check(vkCreateInstance(&create_info, nullptr, out_ptr(vk_instance)));
    
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(vk_instance.get(), &device_count, nullptr);
        {
            auto devices = std::make_unique<VkPhysicalDevice[]>(device_count);
            check(vkEnumeratePhysicalDevices(
                vk_instance.get(), &device_count, devices.get()
            ));
    
            physical_device = devices[0]; // just pick the first one
        }
    }
    current_instance = vk_instance.get();
    
    check(glfwCreateWindowSurface(
        vk_instance.get(), window, nullptr, out_ptr(surface))
    );

    
    // find memory types
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);

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
            physical_device, i, surface.get(), &present_support
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

        // One queue per family
        std::uint32_t queue_count = 
            1 + (graphics_queue_family != present_queue_family);

        const char* enabled_extension_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceFeatures device_features{
            .alphaToOne = VK_TRUE,
        };
        VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = queue_count,
            .pQueueCreateInfos = queue_create_infos,
            .enabledExtensionCount =
                static_cast<uint32_t>(std::size(enabled_extension_names)),
            .ppEnabledExtensionNames = enabled_extension_names,
            .pEnabledFeatures = &device_features,
        };

        if (xr_instance) {
            PFN_xrCreateVulkanDeviceKHR xrCreateVulkanDeviceKHR;
            check(xrGetInstanceProcAddr(
                xr_instance.get(), "xrCreateVulkanDeviceKHR",
                (PFN_xrVoidFunction*)&xrCreateVulkanDeviceKHR
            ));
            VkResult vk_result;
            XrVulkanDeviceCreateInfoKHR xr_vulkan_device_create_info{
                .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
                .next = nullptr,
                .systemId = system_id,
                .pfnGetInstanceProcAddr = &vkGetInstanceProcAddr,
                .vulkanPhysicalDevice = physical_device,
                .vulkanCreateInfo = &create_info,
            };
            check(xrCreateVulkanDeviceKHR(
                xr_instance.get(), &xr_vulkan_device_create_info, 
                out_ptr(vk_device), &vk_result
            ));
            check(vk_result);

        } else {
           check(vkCreateDevice(
                physical_device, &create_info, nullptr, out_ptr(vk_device)
            ));
        }
        current_device = vk_device.get();
    }
    

    if (xr_instance) {
        XrGraphicsBindingVulkan2KHR vulkan_graphics_binding {
            .type = XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR,
            .next = nullptr,
            .instance = vk_instance.get(),
            .physicalDevice = physical_device,
            .device = vk_device.get(),
            .queueFamilyIndex = graphics_queue_family,
            .queueIndex = 0,
        };

        XrSessionCreateInfo session_create_info {
            .type = XR_TYPE_SESSION_CREATE_INFO,
            .next = &vulkan_graphics_binding,
            .systemId = system_id,
        };
        check(xrCreateSession(
            xr_instance.get(), &session_create_info, out_ptr(xr_session)
        ));
     
        uint32_t view_configuration_view_count = 0;
        check(xrEnumerateViewConfigurationViews(
            xr_instance.get(), system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            0, &view_configuration_view_count, nullptr
        ));
        std::vector<XrViewConfigurationView> view_configuration_views(
            view_configuration_view_count
        );
        check(xrEnumerateViewConfigurationViews(
            xr_instance.get(), system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            view_configuration_view_count, &view_configuration_view_count,
            view_configuration_views.data()
        ));

        uint32_t format_count = 0;
        check(xrEnumerateSwapchainFormats(
            xr_session.get(), system_id, &format_count, nullptr
        ));
        std::vector<int64_t> swapchain_formats(format_count);
        check(xrEnumerateSwapchainFormats(
            xr_session.get(), system_id, &format_count, 
            swapchain_formats.data()
        ));
        // TODO: check supported formats

        XrSwapchainCreateInfo swapchain_create_info {
            .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .usageFlags = 
                XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .sampleCount = 1,
            .width = view_configuration_views[0].recommendedImageRectWidth,
            .height = view_configuration_views[0].recommendedImageRectHeight,
            .faceCount = 1,
            .arraySize = 1,
            .mipCount = 1,
        };
        check(xrCreateSwapchain(
            xr_session.get(), &swapchain_create_info, out_ptr(color_swapchain)
        ));

        uint32_t swapchain_image_count = 0;
        check(xrEnumerateSwapchainImages(
            color_swapchain.get(), 0, &swapchain_image_count, nullptr
        ));
        std::vector<XrSwapchainImageBaseHeader> swapchain_images(
            swapchain_image_count
        );
        check(xrEnumerateSwapchainImages(
            color_swapchain.get(), swapchain_image_count, 
            &swapchain_image_count, swapchain_images.data()
        ));
        

        swapchain_create_info = {
            .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .usageFlags = 
                XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
            .format = VK_FORMAT_D24_UNORM_S8_UINT,
            .sampleCount = 1,
            .width = view_configuration_views[0].recommendedImageRectWidth,
            .height = view_configuration_views[0].recommendedImageRectHeight,
            .faceCount = 1,
            .arraySize = 1,
            .mipCount = 1,
        };
        check(xrCreateSwapchain(
            xr_session.get(), &swapchain_create_info, out_ptr(depth_swapchain)
        ));

        swapchain_image_count = 0;
        check(xrEnumerateSwapchainImages(
            depth_swapchain.get(), 0, &swapchain_image_count, nullptr
        ));
        swapchain_images.resize(swapchain_image_count);
        check(xrEnumerateSwapchainImages(
            depth_swapchain.get(), swapchain_image_count, 
            &swapchain_image_count, swapchain_images.data()
        ));
    }

    
    visuals = std::make_unique<::visuals>(
        client, vk_instance.get(), surface.get(), 
        physical_device, vk_device.get(), xr_instance.get(), 
        system_id, xr_session.get(), properties,
        graphics_queue_family, present_queue_family
    );
}

int main(int argc, char *argv[]) {
    start_trace("trace.json", 0);
    unique_glfw glfw;

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    unique_window window(check(glfwCreateWindow(
        1920, 1080, "Hello", nullptr, nullptr
    )));

    hello h(argv);
    
    auto visuals = std::make_unique<vk_glfw_visuals>(window.get(), *h.client);

    ::input input{};

    double previous_time = glfwGetTime();

    while (!glfwWindowShouldClose(window.get())) {
        double time = glfwGetTime();
        float delta = time - previous_time;
        previous_time = time;

        update(input, window.get(), delta);

        try {
            h.update(
                input, visuals->xr_instance.get(), visuals->xr_session.get()
            );
        } catch (xr_error& e) {
            std::printf("XR error: %s\n", e.what());
            visuals.reset();
            visuals = 
                std::make_unique<vk_glfw_visuals>(window.get(), *h.client);
        }
        visuals->visuals->draw(*h.client);

        glfwPollEvents();
    }

    return 0;
}
