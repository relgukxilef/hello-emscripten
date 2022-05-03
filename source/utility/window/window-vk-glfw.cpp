#include "window.h"

#include <stdexcept>
#include <iostream>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#define GLFW_VULKAN_STATIC
#include <GLFW/glfw3.h>

#include "../resource.h"
#include "../vulkan_resource.h"
#include "../out_ptr.h"

void glfw_check(int code) {
    if (code == GLFW_TRUE) {
        return;
    } else {
        throw std::runtime_error("Failed to initialize GLFW");
    }
}

struct unique_glfw {
    unique_glfw() { glfw_check(glfwInit()); }
    ~unique_glfw() { glfwTerminate(); }
};

void glfw_delete_window(GLFWwindow** window) {
    glfwDestroyWindow(*window);
}

using unique_window = unique_resource<GLFWwindow*, glfw_delete_window>;

struct window_internal {
    window_internal();

    unique_glfw glfw;
    unique_instance instance;
    unique_window glfw_window;
    unique_surface surface;
};

window::window() : internal(std::make_unique<window_internal>()) {}

window::~window() {}

VkInstance window::get_instance() {
    return internal->instance.get();
}

VkSurfaceKHR window::get_surface() {
    return internal->surface.get();
}

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

window_internal::window_internal() {
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

    VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debug_utils_messenger_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(extension_count),
        .ppEnabledExtensionNames = extensions.get(),
    };
    check(vkCreateInstance(&create_info, nullptr, out_ptr(instance)));
    current_instance = instance.get();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfw_window = glfwCreateWindow(1280, 720, "Test", nullptr, nullptr);
    check(glfwCreateWindowSurface(
        instance.get(), glfw_window.get(), nullptr, out_ptr(surface))
    );
}
