#include "view.h"

#include <algorithm>
#include <cinttypes>
#include <memory>

#include "visuals.h"

#include "../utility/out_ptr.h"

view::view(visuals &v, VkInstance instance, VkSurfaceKHR surface) {
    // create swap chains
    uint32_t format_count = 0, present_mode_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        v.physical_device, surface, &format_count, nullptr
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        v.physical_device, surface, &present_mode_count, nullptr
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
        v.physical_device, surface, &format_count, formats.get()
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        v.physical_device, surface, &present_mode_count, present_modes.get()
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
        v.physical_device, surface, &capabilities
    ));

    unsigned width = capabilities.currentExtent.width;
    unsigned height = capabilities.currentExtent.height;

    surface_extent = {
        std::max(
            std::min<uint32_t>(width, capabilities.maxImageExtent.width),
            capabilities.minImageExtent.width
        ),
        std::max(
            std::min<uint32_t>(height, capabilities.maxImageExtent.height),
            capabilities.minImageExtent.height
        )
    };

    {
        uint32_t queue_family_indices[]{
            v.graphics_queue_family, v.present_queue_family
        };
        VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = capabilities.minImageCount,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = surface_extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount =
                static_cast<uint32_t>(std::size(queue_family_indices)),
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

    {
        auto attachments = {
            VkAttachmentDescription{
                .format = surface_format.format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
        };
        auto attachment_references = {
            VkAttachmentReference{
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
        };
        auto subpasses = {
            VkSubpassDescription{
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount =
                    static_cast<uint32_t>(attachment_references.size()),
                .pColorAttachments = attachment_references.begin(),
            },
        };
        auto subpass_dependencies = {
            VkSubpassDependency{
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            },
        };
        VkRenderPassCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.begin(),
            .subpassCount = static_cast<uint32_t>(subpasses.size()),
            .pSubpasses = subpasses.begin(),
            .dependencyCount =
                static_cast<uint32_t>(subpass_dependencies.size()),
            .pDependencies = subpass_dependencies.begin(),
        };
        check(vkCreateRenderPass(
            v.device.get(), &create_info, nullptr, out_ptr(render_pass)
        ));
    }

    // create pipelines
    {
        VkPipelineShaderStageCreateInfo stage_create_infos[]{
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = v.vertex_shader_module.get(),
                .pName = "main",
            }, /*{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = v.fragment_shader_module.get(),
                .pName = "main",
            }*/
        };
        VkPipelineVertexInputStateCreateInfo input_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        };
        VkPipelineInputAssemblyStateCreateInfo assembly_state_create_info{
            .sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };
        VkPipelineViewportStateCreateInfo viewport_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            // number of viewports and scissors is still relevant
            .viewportCount = 1,
            .pViewports = nullptr, // dynamic
            .scissorCount = 1,
            .pScissors = nullptr, // dynamic
        };
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
        };
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
        };
        VkPipelineColorBlendAttachmentState color_blend_attachment_state{
            .blendEnable = VK_FALSE,
            .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment_state,
        };
        VkDynamicState dynamic_state[]{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = (size_t)std::size(dynamic_state),
            .pDynamicStates = dynamic_state,
        };
        VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
        };
        VkGraphicsPipelineCreateInfo pipeline_create_info{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = (size_t)std::size(stage_create_infos),
            .pStages = stage_create_infos,
            .pVertexInputState = &input_state_create_info,
            .pInputAssemblyState = &assembly_state_create_info,
            .pViewportState = &viewport_state_create_info,
            .pRasterizationState = &rasterization_state_create_info,
            .pMultisampleState = &multisample_state_create_info,
            .pDepthStencilState = &depth_stencil_info,
            .pColorBlendState = &color_blend_state_create_info,
            .pDynamicState = &dynamic_state_create_info,
            .layout = v.pipeline_layout.get(),
            .renderPass = render_pass.get(),
            .subpass = 0,
        };
        check(vkCreateGraphicsPipelines(
            v.device.get(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr,
            out_ptr(pipeline)
        ));
    }

    images = std::make_unique<image[]>(image_count);

    for (uint32_t i = 0; i < image_count; i++) {
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

        {
            VkImageViewCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = swapchain_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = surface_format.format,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };
            check(vkCreateImageView(
                v.device.get(), &create_info, nullptr,
                out_ptr(image.image_view)
            ));
        }

        {
            auto attachments = {
                image.image_view.get(),
            };
            VkFramebufferCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = render_pass.get(),
                .attachmentCount =
                    static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.begin(),
                .width = surface_extent.width,
                .height = surface_extent.height,
                .layers = 1,
            };
            check(vkCreateFramebuffer(
                v.device.get(), &create_info, nullptr,
                out_ptr(image.framebuffer)
            ));
        }

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

        VkClearValue clearValue[]{
            {{{0.929f, 0.788f, 0.318f, 1.0f}}}, // color
        };
        VkRenderPassBeginInfo render_pass_begin_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass.get(),
            .framebuffer = image.framebuffer.get(),
            .renderArea{
                .offset = {0, 0},
                .extent = surface_extent,
            },
            .clearValueCount =
                static_cast<uint32_t>(std::size(clearValue)),
            .pClearValues = clearValue,
        };
        vkCmdBeginRenderPass(
            image.draw_command_buffer, &render_pass_begin_info,
            VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdEndRenderPass(image.draw_command_buffer);

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

        VkFence fences[] = {images[image_index].draw_finished_fence.get()};

        check(vkWaitForFences(
            v.device.get(), 1, fences,
            VK_TRUE, ~0ul
        ));
        check(vkResetFences(
            v.device.get(), 1, fences
        ));

        VkSemaphore wait_semaphores[] =
            {v.swapchain_image_ready_semaphore.get()};
        VkSemaphore signal_semaphores[] =
            {images[image_index].draw_finished_semaphore.get()};
        VkCommandBuffer buffers[] = {images[image_index].draw_command_buffer};
        VkPipelineStageFlags wait_stage[] =
            {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = wait_semaphores,
            .pWaitDstStageMask = wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = buffers,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signal_semaphores,
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
