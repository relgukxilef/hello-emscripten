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

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*
) noexcept {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::fprintf(
            stderr, "Validation layer error: %s\n", callback_data->pMessage
        );
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::fprintf(
            stderr, "Validation layer warning: %s\n", callback_data->pMessage
        );
    }

    return VK_FALSE;
}

int main(int argc, char *argv[]) {
    start_trace("trace.json", 0);
    unique_glfw glfw;

    glfwSetErrorCallback(error_callback);

    // set up error handling
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

    unique_instance vk_instance;
    VkApplicationInfo application_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .applicationVersion = 0,
        .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };
    VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debug_utils_messenger_create_info,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extension_count),
        .ppEnabledExtensionNames = extensions.get(),
    };

    unique_xr_instance xr_instance;
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
        check(xrGetSystem(xr_instance.get(), &system_get_info, &system_id));

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

    } catch (xr_error& e) {
        std::printf("Starting without VR support.\n");
        check(vkCreateInstance(&create_info, nullptr, out_ptr(vk_instance)));
    }
    
    current_instance = vk_instance.get();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    unique_window window(check(glfwCreateWindow(
        1920, 1080, "Hello", nullptr, nullptr
    )));

    unique_surface surface;
    check(glfwCreateWindowSurface(
        vk_instance.get(), window.get(), nullptr, out_ptr(surface))
    );

    hello h(argv);
    auto visuals = std::make_unique<::visuals>(
        *h.client, vk_instance.get(), surface.get()
    );

    ::input input{};

    double previous_time = glfwGetTime();

    while (!glfwWindowShouldClose(window.get())) {
        double time = glfwGetTime();
        float delta = time - previous_time;
        previous_time = time;

        update(input, window.get(), delta);

        h.update(input);
        visuals->draw(*h.client);

        glfwPollEvents();
    }

    return 0;
}
