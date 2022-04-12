#include "window.h"

#include <stdexcept>

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

window_internal::window_internal() {
    uint32_t extension_count = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = static_cast<uint32_t>(extension_count),
        .ppEnabledExtensionNames = extensions,
    };
    check(vkCreateInstance(&create_info, nullptr, out_ptr(instance)));
    current_instance = instance.get();

    glfw_window = glfwCreateWindow(1280, 720, "Test", nullptr, nullptr);
    check(glfwCreateWindowSurface(
        instance.get(), glfw_window.get(), nullptr, out_ptr(surface))
    );
}
