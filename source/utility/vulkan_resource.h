#pragma once

#include <stdexcept>

#include <vulkan/vulkan.h>

#include "resource.h"

extern VkInstance current_instance;
extern VkDevice current_device;

struct vulkan_error : public std::exception {
    vulkan_error(VkResult result) noexcept;

    const char *what() const noexcept override;

    VkResult result;
};

inline vulkan_error::vulkan_error(VkResult result) noexcept : result(result) {}

inline void check(VkResult success) {
    if (success == VK_SUCCESS) {
        return;
    } else {
        throw vulkan_error(success);
    }
}

template<typename T, auto Deleter>
void vulkan_delete(T* t) {
    Deleter(current_device, *t, nullptr);
}

inline void vulkan_delete_instance(VkInstance* device) {
    vkDestroyInstance(*device, nullptr);
}

inline void vulkan_delete_debug_utils_messenger(
    VkDebugUtilsMessengerEXT* messenger
) {
    // TODO: is there a better place to load the function?
    auto vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            current_instance, "vkDestroyDebugUtilsMessengerEXT"
    );
    vkDestroyDebugUtilsMessengerEXT(current_instance, *messenger, nullptr);
}

inline void vulkan_delete_device(VkDevice* device) {
    vkDestroyDevice(*device, nullptr);
}

inline void vulkan_delete_surface(VkSurfaceKHR* surface) {
    vkDestroySurfaceKHR(current_instance, *surface, nullptr);
}

inline void vulkan_wait_and_delete_fence(VkFence* fence) {
    VkResult result = vkWaitForFences(current_device, 1, fence, VK_TRUE, ~0ul);
    // fence needs to be cleaned up regardless of whether waiting succeeded
    vkDestroyFence(current_device, *fence, nullptr);
    if (result != VK_SUCCESS && result != VK_TIMEOUT)
        check(result);
}

template<typename T, auto Deleter>
using unique_vulkan_resource =
    unique_resource<T, vulkan_delete<T, Deleter>, VK_NULL_HANDLE>;


typedef unique_resource<
    VkDebugUtilsMessengerEXT, vulkan_delete_debug_utils_messenger,
    VK_NULL_HANDLE
> unique_debug_utils_messenger;

typedef unique_resource<VkInstance, vulkan_delete_instance, nullptr>
    unique_instance;

typedef unique_resource<VkDevice, vulkan_delete_device, nullptr>
    unique_device;

typedef unique_resource<VkSurfaceKHR, vulkan_delete_surface, VK_NULL_HANDLE>
    unique_surface;

typedef unique_vulkan_resource<VkShaderModule, vkDestroyShaderModule>
    unique_shader_module;

typedef unique_vulkan_resource<VkFramebuffer, vkDestroyFramebuffer>
    unique_framebuffer;

typedef unique_vulkan_resource<VkPipeline, vkDestroyPipeline>
    unique_pipeline;

typedef unique_vulkan_resource<VkImage, vkDestroyImage> unique_image;

typedef unique_vulkan_resource<VkImageView, vkDestroyImageView>
    unique_image_view;

typedef unique_vulkan_resource<VkSampler, vkDestroySampler>
    unique_sampler;

typedef unique_vulkan_resource<VkPipelineLayout, vkDestroyPipelineLayout>
    unique_pipeline_layout;

typedef unique_vulkan_resource<
    VkDescriptorSetLayout, vkDestroyDescriptorSetLayout
> unique_descriptor_set_layout;

typedef unique_vulkan_resource<VkRenderPass, vkDestroyRenderPass>
    unique_render_pass;

typedef unique_vulkan_resource<VkCommandPool, vkDestroyCommandPool>
    unique_command_pool;

typedef unique_resource<VkFence, vulkan_wait_and_delete_fence> unique_fence;

typedef unique_vulkan_resource<VkSemaphore, vkDestroySemaphore>
    unique_semaphore;

typedef unique_vulkan_resource<VkSwapchainKHR, vkDestroySwapchainKHR>
    unique_swapchain;

typedef unique_vulkan_resource<VkBuffer, vkDestroyBuffer>
    unique_buffer;

typedef unique_vulkan_resource<VkDeviceMemory, vkFreeMemory>
    unique_device_memory;

typedef unique_vulkan_resource<VkDescriptorPool, vkDestroyDescriptorPool>
    unique_descriptor_pool;
