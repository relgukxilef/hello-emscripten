#pragma once

#include <memory>

#include <GLFW/glfw3.h>

#include "state/input.h"

void update(::input& input, GLFWwindow *window, float delta);

struct graphics_binding {
    graphics_binding(GLFWwindow *window);
    ~graphics_binding();
    const void* get_graphics_binding();

    std::unique_ptr<struct graphics_binding_info> info;
};
