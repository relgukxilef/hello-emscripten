#include "view.h"

#include <algorithm>
#include <cinttypes>
#include <memory>

#include "visuals.h"

#include "../utility/out_ptr.h"

view::view(visuals &v) {
    // create swap chains
    uint32_t format_count = 0, present_mode_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        v.physical_device, v.surface, &format_count, nullptr
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        v.physical_device, v.surface, &present_mode_count, nullptr
    );
    if (format_count == 0) {
        throw std::runtime_error("no surface formats supported");
    }
    if (present_mode_count == 0) {
        throw std::runtime_error("no surface present modes supported");
    }
    auto formats = std::make_unique<VkSurfaceFormatKHR[]>(format_count);
    auto present_modes =
        std::make_unique<VkPresentModeKHR[]>(present_mode_count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        v.physical_device, v.surface, &format_count, formats.get()
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        v.physical_device, v.surface, &present_mode_count, present_modes.get()
    );

    VkSurfaceFormatKHR surface_format;

    surface_format = formats[0];
    for (auto i = 0u; i < format_count; i++) {
        auto format = formats[i];
        if (
            format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            surface_format = format;
        }
    }

    {
        VkCommandPoolCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = v.graphics_queue_family,
        };
        check(vkCreateCommandPool(
            v.device.get(), &create_info, nullptr, out_ptr(command_pool)
        ));
    }

    // view-specific resources
    VkSurfaceCapabilitiesKHR capabilities;
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        v.physical_device, v.surface, &capabilities
    ));

    unsigned width = capabilities.currentExtent.width;
    unsigned height = capabilities.currentExtent.height;

    VkExtent2D extent = {
        std::max(
            std::min<std::uint32_t>(width, capabilities.maxImageExtent.width),
            capabilities.minImageExtent.width
        ),
        std::max(
            std::min<std::uint32_t>(height, capabilities.maxImageExtent.height),
            capabilities.minImageExtent.height
        )
    };

    {
        uint32_t queue_family_indices[]{
            v.graphics_queue_family, v.present_queue_family
        };
        VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = v.surface,
            .minImageCount = capabilities.minImageCount,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = std::size(queue_family_indices),
            .pQueueFamilyIndices = queue_family_indices,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            // fifo has the widest support
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };
        check(vkCreateSwapchainKHR(
            v.device.get(), &create_info, nullptr, out_ptr(swapchain)
        ));
    }

    uint32_t image_count;
    check(vkGetSwapchainImagesKHR(
        v.device.get(), swapchain.get(), &image_count, nullptr
    ));

    auto swapchain_images = std::make_unique<VkImage[]>(image_count);

    check(vkGetSwapchainImagesKHR(
        v.device.get(), swapchain.get(), &image_count, swapchain_images.get()
    ));

    images = std::make_unique<image[]>(image_count);

    for (auto i = 0; i < image_count; i++) {
        ::image& image = images[i];

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        check(vkCreateSemaphore(
            v.device.get(), &semaphore_info, nullptr,
            out_ptr(image.draw_finished_semaphore)
        ));

        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        check(vkCreateFence(
            v.device.get(), &fence_info, nullptr,
            out_ptr(image.draw_finished_fence)
        ));

        VkCommandBufferAllocateInfo command_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool.get(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        check(vkAllocateCommandBuffers(
            v.device.get(), &command_buffer_info, &image.draw_command_buffer
        ));

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        check(vkBeginCommandBuffer(image.draw_command_buffer, &begin_info));

        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchain_images[0],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        vkCmdPipelineBarrier(
            image.draw_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr, 0, nullptr, 1, &barrier
        );

        check(vkEndCommandBuffer(image.draw_command_buffer));
    }
}

VkResult view::draw(visuals &v) {
    uint32_t image_index;
        VkResult result = vkAcquireNextImageKHR(
            v.device.get(), swapchain.get(), ~0ul,
            v.swapchain_image_ready_semaphore.get(),
            VK_NULL_HANDLE, &image_index
        );
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            return result;
        }
        check(result);

        check(vkWaitForFences(
            v.device.get(), 1, &images[image_index].draw_finished_fence.get(),
            VK_TRUE, ~0ul
        ));
        check(vkResetFences(
            v.device.get(), 1, &images[image_index].draw_finished_fence.get()
        ));

        // TODO: maybe move this to image::render
        VkPipelineStageFlags wait_stage =
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &v.swapchain_image_ready_semaphore.get(),
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &images[image_index].draw_command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores =
                &images[image_index].draw_finished_semaphore.get(),
        };
        check(vkQueueSubmit(
            v.graphics_queue, 1, &submitInfo,
            images[image_index].draw_finished_fence.get()
        ));

        VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = 
                &images[image_index].draw_finished_semaphore.get(),
            .swapchainCount = 1,
            .pSwapchains = &swapchain.get(),
            .pImageIndices = &image_index,
        };
        result = vkQueuePresentKHR(v.present_queue, &present_info);
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            return result;
        }
        check(result);

        return VK_SUCCESS;
}
