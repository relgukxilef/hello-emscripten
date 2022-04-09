#include "window.h"

#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#define GLFW_VULKAN_STATIC
#include <GLFW/glfw3.h>

#include "../utility/resource.h"
#include "../utility/vulkan_resource.h"

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

    VkSurfaceKHR get_surface();

    unique_glfw glfw;
    unique_window w;
};

VkSurfaceKHR window::get_surface() {
    return internal->get_surface();
}

window_internal::window_internal() {
    w = glfwCreateWindow(1280, 720, "Test", nullptr, nullptr);
}

VkSurfaceKHR window_internal::get_surface() {
    return {}; // TODO
}