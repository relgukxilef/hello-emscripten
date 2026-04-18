#include <cstdio>
#include <memory>

#include <GLFW/glfw3.h>
#include <glad/gles2.h>
#include <vulkangl/vulkangl.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "main-glfw.h"
#include "hello.h"
#include "visuals/visuals.h"
#include "utility/resource.h"
#include "utility/trace.h"
#include "utility/xr_resource.h"
#include "utility/out_ptr.h"
#include "vulkan/vulkan_core.h"

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

    VkInstance instance;
    vkCreateInstance(nullptr, nullptr, &instance);

    vglSetDeviceMemory(256 * 1024 * 1024);
    vglSetHostMemory(256 * 1024 * 1024);

    int width, height;
    glfwGetWindowSize(window.get(), &width, &height);
    vglSetCurrentSurfaceExtent(
        { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
    );

    XrSystemId system_id = {};
    std::vector<VkImage> color_images, depth_images;

    unique_xr_instance xr_instance;
    unique_xr_session xr_session;
    XrExtent2Di xr_extent {};
    VkFormat surface_format {};
    unique_xr_swapchain color_swapchain;
    
    try {
        const char* xr_extensions[]{
            XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
        };
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
            .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
        };
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

        XrGraphicsRequirementsOpenGLKHR requirements {
            .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
        };
        PFN_xrGetOpenGLGraphicsRequirementsKHR
        xrGetOpenGLGraphicsRequirementsKHR;
        check(xrGetInstanceProcAddr(
            xr_instance.get(), "xrGetOpenGLGraphicsRequirementsKHR",
            (PFN_xrVoidFunction*)&xrGetOpenGLGraphicsRequirementsKHR
        ));
        check(xrGetOpenGLGraphicsRequirementsKHR(
            xr_instance.get(), system_id, &requirements
        ));

        graphics_binding binding(window.get());
        
        XrSessionCreateInfo session_create_info {
            .type = XR_TYPE_SESSION_CREATE_INFO,
            .next = binding.get_graphics_binding(),
            .systemId = system_id,
        };
        check(xrCreateSession(
            xr_instance.get(), &session_create_info, out_ptr(xr_session)
        ));

        uint32_t view_configuration_view_count = 0;
        check(xrEnumerateViewConfigurationViews(
            xr_instance.get(), system_id, 
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            0, &view_configuration_view_count, nullptr
        ));
        std::vector<XrViewConfigurationView> view_configuration_views(
            view_configuration_view_count
        );
        for (auto& v : view_configuration_views) {
            v.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        }
        check(xrEnumerateViewConfigurationViews(
            xr_instance.get(), system_id, 
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            view_configuration_view_count, &view_configuration_view_count,
            view_configuration_views.data()
        ));
        // TODO: views could have different size
        xr_extent = {
            .width = 
                int32_t(view_configuration_views[0].recommendedImageRectWidth),
            .height = 
                int32_t(view_configuration_views[0].recommendedImageRectHeight),
        };

        uint32_t format_count = 0;
        check(xrEnumerateSwapchainFormats(
            xr_session.get(), 0, &format_count, nullptr
        ));
        std::vector<int64_t> formats(format_count);
        check(xrEnumerateSwapchainFormats(
            xr_session.get(), format_count, &format_count, 
            formats.data()
        ));
        // TODO: client should decide format, but swapchain creation is platform
        // specific
        GLuint gl_surface_format = formats[0];
        for (auto i = 1u; i < format_count; i++) {
            auto format = formats[i];
            if (format == GL_SRGB8_ALPHA8) {
                gl_surface_format = format;
            }
        }
        surface_format = vglVkFormatFromGL(gl_surface_format);

        XrSwapchainCreateInfo swapchain_create_info {
            .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .usageFlags = 
                XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
            .format = int64_t(gl_surface_format),
            .sampleCount = 1,
            .width = uint32_t(xr_extent.width),
            .height = uint32_t(xr_extent.height),
            .faceCount = 1,
            .arraySize = 2,
            .mipCount = 1,
        };
        check(xrCreateSwapchain(
            xr_session.get(), &swapchain_create_info, out_ptr(color_swapchain)
        ));

        uint32_t swapchain_image_count = 0;
        check(xrEnumerateSwapchainImages(
            color_swapchain.get(), 0, &swapchain_image_count, nullptr
        ));
        std::vector<XrSwapchainImageOpenGLKHR> swapchain_images(
            swapchain_image_count, 
            XrSwapchainImageOpenGLKHR { 
                .type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR 
            }
        );
        check(xrEnumerateSwapchainImages(
            color_swapchain.get(), swapchain_image_count, 
            &swapchain_image_count, 
            reinterpret_cast<XrSwapchainImageBaseHeader*>(
                swapchain_images.data()
            )
        ));
        color_images.resize(swapchain_image_count);
        for (auto i = 0u; i < swapchain_image_count; i++) {
            // TODO: create new swapchain if recommended resolution changes
            auto swapchain_image = vglVkImageFromGL(
                swapchain_images[i].image, gl_surface_format, 
                xr_extent.width, xr_extent.height, 
                2
            );
            color_images[i] = swapchain_image;
        }

    } catch (std::exception& e) {
        std::printf("Starting without VR support. (%s)\n", e.what());
        
    }

    hello h(argv);

    VkInstance vk_instance = vglCreateInstanceForGL();
    VkPhysicalDevice vk_physical_device;
    uint32_t device_count = 1;
    vkEnumeratePhysicalDevices(vk_instance, &device_count, &vk_physical_device);
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &properties);
    VkDevice vk_device;
    vkCreateDevice(
        vk_physical_device, nullptr, nullptr, &vk_device
    );
    auto visuals = std::make_unique<::visuals>(
        *h.client, platform{
            vk_instance, vglCreateSurfaceForGL(),
            vk_physical_device, vk_device, 
            properties, 0, 0,
            
            xr_instance.get(), 
            system_id, xr_session.get(), color_swapchain.get(),
            std::move(color_images),
            xr_extent, surface_format
        }
    );

    ::input input {};

    double previous_time = glfwGetTime();

    while (!glfwWindowShouldClose(window.get())) {
        double time = glfwGetTime();
        float delta = double(time - previous_time);
        previous_time = time;

        glfwGetWindowSize(window.get(), &width, &height);
        vglSetCurrentSurfaceExtent(
            { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
        );

        update(input, window.get(), delta);

        h.update(input, xr_instance.get(), xr_session.get());
        visuals->draw(*h.client);

        glfwSwapBuffers(window.get());

        glfwPollEvents();
    }

    return 0;
}
