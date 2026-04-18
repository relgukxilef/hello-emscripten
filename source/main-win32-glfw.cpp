#include "main-glfw.h"

#include <windows.h>
#include <vulkan/vulkan.h>

#define XR_USE_PLATFORM_WIN32
#include <openxr/openxr_platform.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>

struct graphics_binding_info {
    XrGraphicsBindingOpenGLWin32KHR info;
};

graphics_binding::graphics_binding(GLFWwindow *window) {
    HDC hDC = GetDC(glfwGetWin32Window(window));
    HGLRC hGLRC = glfwGetWGLContext(window);

    info.reset(new graphics_binding_info);
    info->info = {
        .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
        .hDC = hDC,
        .hGLRC = hGLRC,
    };
}

graphics_binding::~graphics_binding() = default;

const void* graphics_binding::get_graphics_binding() {
    return &info->info;
}
