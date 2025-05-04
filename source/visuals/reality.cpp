#include "reality.h"

#include "../utility/out_ptr.h"

reality::reality(XrInstance instance, platform platform) {
    XrGraphicsBindingVulkan2KHR vulkan_graphics_binding {
        .type = XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR,
        .next = nullptr,
        .instance = instance,
        .physicalDevice = nullptr, // TODO: get physical device
        .device = nullptr, // TODO: get device
        .queueFamilyIndex = 0, // TODO: get queue family index
        .queueIndex = 0, // TODO: get queue index
    };
    // TODO: other bindings

    XrSessionCreateInfo session_create_info {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = nullptr,
        .createFlags = 0,
        .systemId = system_id,
    };
    check(xrCreateSession(
        instance, &session_create_info, out_ptr(xr_session)
    ));
}
