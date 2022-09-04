#pragma once

#include <GLFW/glfw3.h>

#include "state/input.h"

void update(::input& input, GLFWwindow *window, float delta);
