#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <memory>

#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include <vulkangl/vulkangl.h>

#include "main-glfw.h"
#include "hello.h"
#include "utility/resource.h"
#include "utility/trace.h"
#include "network/websocket-boost.h"

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

int main(int argc, char *argv[]) {
    start_trace("trace.json", 0);

    unique_glfw glfw;

    glfwSetErrorCallback(error_callback);

    unique_window window(check(glfwCreateWindow(
        1920, 1080, "Hello", nullptr, nullptr
    )));

    glfwMakeContextCurrent(window.get());

    gladLoadGLES2(glfwGetProcAddress);

    vglSetDeviceMemory(256 * 1024 * 1024);
    vglSetHostMemory(256 * 1024 * 1024);

    int width, height;
    glfwGetWindowSize(window.get(), &width, &height);
    vglSetCurrentSurfaceExtent(
        { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
    );

    boost_websockets websockets;

    hello h(
        argv, vglCreateInstanceForGL(), vglCreateSurfaceForGL(), 
        &websockets.websockets
    );

    ::input input {};
    
    double previous_time = glfwGetTime();

    while (!glfwWindowShouldClose(window.get())) {
        double time = glfwGetTime();
        float delta = time - previous_time;
        previous_time = time;

        glfwGetWindowSize(window.get(), &width, &height);
        vglSetCurrentSurfaceExtent(
            { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
        );

        update(input, window.get(), delta);

        h.update(input);
        websockets.update();

        h.draw(vglCreateInstanceForGL(), vglCreateSurfaceForGL());

        glfwSwapBuffers(window.get());

        glfwPollEvents();
    }

    return 0;
}
