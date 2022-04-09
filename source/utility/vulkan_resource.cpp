#include "vulkan_resource.h"

VkInstance current_instance = VK_NULL_HANDLE;
VkDevice current_device = VK_NULL_HANDLE;

const char *vulkan_error::what() const noexcept {
    switch (result) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "A host memory allocation has failed.";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "A device memory allocation has failed.";
    case VK_ERROR_DEVICE_LOST:
        return "The logical or physical device has been lost.";
    default:
        return "Other vulkan error.";
    }
}
