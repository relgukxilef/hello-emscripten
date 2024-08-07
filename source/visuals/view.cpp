#include "view.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

#include "visuals.h"

#include "../utility/out_ptr.h"
#include "../utility/math.h"
#include "../utility/trace.h"

void record_command_buffer(
    client& client, visuals& visuals, view& view, image& image,
    VkPipelineLayout pipeline_layout
) {
    scope_trace trace;
    // The old command buffer is reset before this, so writing descriptors is ok
    // update descriptors
    auto image_info =
        std::make_unique<VkDescriptorImageInfo[]>(view.descriptor_set_count);

    auto write_descriptor_sets = std::make_unique<VkWriteDescriptorSet[]>(
        view.descriptor_set_count
    );

    auto primitive = 0u;
    for (auto j = 0u; j < client.world_model.primitives.size(); j++) {
        if (primitive >= view.descriptor_set_count)
            break;
        image_info[primitive] = {
            .sampler = visuals.default_sampler.get(),
            .imageView = visuals.images[
                std::min<unsigned>(
                    client.test_model.images.size() +
                    client.world_model.primitives[j].image_index,
                    visuals.images.size() - 1
                )
            ].view.get(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        write_descriptor_sets[primitive] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = image.descriptor_sets[primitive],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_info[primitive],
        };
        primitive++;
    }
    for (auto i = 0u; i < client.users.position.size(); i++) {
        for (auto j = 0u; j < client.test_model.primitives.size(); j++) {
            if (primitive >= view.descriptor_set_count)
                break;
            image_info[primitive] = {
                .sampler = visuals.default_sampler.get(),
                .imageView = visuals.images[
                    std::min<unsigned>(
                        client.test_model.primitives[j].image_index,
                        visuals.images.size() - 1
                    )
                ].view.get(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            write_descriptor_sets[primitive] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = image.descriptor_sets[primitive],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info[primitive],
            };
            primitive++;
        }
    }

    vkUpdateDescriptorSets(
        visuals.device.get(), primitive,
        write_descriptor_sets.get(), 0, nullptr
    );


    VkSurfaceCapabilitiesKHR capabilities = view.capabilities;
    unsigned
        width = capabilities.currentExtent.width,
        height = capabilities.currentExtent.height;

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    check(vkBeginCommandBuffer(image.draw_command_buffer, &begin_info));

    VkClearValue clearValue[]{
        {.color{{0.929f, 0.788f, 0.318f, 1.0f}}},
        {.depthStencil{1.0f, 0}},
        {.color{{0.929f, 0.788f, 0.318f, 1.0f}}},
    };
    VkRenderPassBeginInfo render_pass_begin_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = view.render_pass.get(),
        .framebuffer = image.framebuffer.get(),
        .renderArea{
            .offset = {0, 0},
            .extent = view.surface_extent,
        },
        .clearValueCount =
            static_cast<uint32_t>(std::size(clearValue)),
        .pClearValues = clearValue,
    };
    vkCmdBeginRenderPass(
        image.draw_command_buffer, &render_pass_begin_info,
        VK_SUBPASS_CONTENTS_INLINE
    );

    vkCmdBindPipeline(
        image.draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        view.pipeline.get()
    );

    VkViewport viewport{
        .x = 0,
        .y = 0,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(image.draw_command_buffer, 0, 1, &viewport);

    VkRect2D scissors{
        .offset = {0, 0},
        .extent = capabilities.currentExtent,
    };
    vkCmdSetScissor(image.draw_command_buffer, 0, 1, &scissors);

    VkDescriptorSet descriptor_sets[] {
        image.descriptor_sets[0],
    };

    VkBuffer vertex_buffers[] = {
        visuals.vertex_buffer.get(), visuals.vertex_buffer.get(),
        visuals.vertex_buffer.get(),
    };

    vkCmdBindDescriptorSets(
        image.draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout, 0, 1, descriptor_sets, 0, nullptr
    );

    primitive = 0u;

    // draw world
    {
        auto& model = visuals.models[1];

        VkDeviceSize offsets[] = {
            model.position_offset,
            model.normal_offset,
            model.texture_coordinate_offset,
        };
        vkCmdBindVertexBuffers(
            image.draw_command_buffer, 0, std::size(vertex_buffers),
            vertex_buffers, offsets
        );
        vkCmdBindIndexBuffer(
            image.draw_command_buffer, visuals.index_buffer.get(),
            model.indices_offset, VK_INDEX_TYPE_UINT32
        );
    }

    for (auto j = 0u; j < client.world_model.primitives.size(); j++) {
        if (primitive >= view.descriptor_set_count)
            break;
        VkDescriptorSet descriptor_sets[] {
            image.descriptor_sets[primitive],
        };
        vkCmdBindDescriptorSets(
            image.draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout, 0, 1, descriptor_sets, 0, nullptr
        );

        vkCmdDrawIndexed(
            image.draw_command_buffer,
            client.world_model.primitives[j].face_size,
            1,
            client.world_model.primitives[j].face_begin,
            client.world_model.primitives[j].vertex_begin,
            0
        );

        primitive++;
    }

    // draw users
    {
        auto& model = visuals.models[0];

        VkDeviceSize offsets[] = {
            model.position_offset,
            model.normal_offset,
            model.texture_coordinate_offset,
        };
        vkCmdBindVertexBuffers(
            image.draw_command_buffer, 0, std::size(vertex_buffers),
            vertex_buffers, offsets
        );
        vkCmdBindIndexBuffer(
            image.draw_command_buffer, visuals.index_buffer.get(),
            model.indices_offset, VK_INDEX_TYPE_UINT32
        );
    }

    for (auto i = 0u; i < client.users.position.size(); i++) {
        for (auto j = 0u; j < client.test_model.primitives.size(); j++) {
            if (primitive >= view.descriptor_set_count)
                break;
            VkDescriptorSet descriptor_sets[] {
                image.descriptor_sets[primitive],
            };
            vkCmdBindDescriptorSets(
                image.draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_layout, 0, 1, descriptor_sets, 0, nullptr
            );

            vkCmdDrawIndexed(
                image.draw_command_buffer,
                client.test_model.primitives[j].face_size,
                1,
                client.test_model.primitives[j].face_begin,
                client.test_model.primitives[j].vertex_begin,
                0
            );

            primitive++;
        }
    }

    vkCmdEndRenderPass(image.draw_command_buffer);

    check(vkEndCommandBuffer(image.draw_command_buffer));
}

view::view(client& c, visuals &v, VkInstance instance, VkSurfaceKHR surface) {
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

    // view-specific resources
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        v.physical_device, surface, &capabilities
    ));

    unsigned width = capabilities.currentExtent.width;
    unsigned height = capabilities.currentExtent.height;
    printf("Set up view of size %ix%i\n", width, height);

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

    // create descriptor pool
    {
        VkDescriptorPoolSize pool_sizes[] {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = image_count * descriptor_set_count,
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = image_count * descriptor_set_count,
            },
        };
        VkDescriptorPoolCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = image_count * descriptor_set_count,
            .poolSizeCount = (uint32_t)std::size(pool_sizes),
            .pPoolSizes = pool_sizes,
        };
        check(vkCreateDescriptorPool(
            v.device.get(), &create_info, nullptr, out_ptr(descriptor_pool))
        );
    }

    // create render passes
    {
        auto attachments = {
            VkAttachmentDescription{
                .format = surface_format.format,
                .samples = VK_SAMPLE_COUNT_2_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
            VkAttachmentDescription{
                .format = VK_FORMAT_D24_UNORM_S8_UINT,
                .samples = VK_SAMPLE_COUNT_2_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
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
        auto color_attachment_references = {
            VkAttachmentReference{
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
        };
        VkAttachmentReference depth_attachment_reference{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        VkAttachmentReference resolve_attachment_reference{
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        auto subpasses = {
            VkSubpassDescription{
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount =
                    static_cast<uint32_t>(color_attachment_references.size()),
                .pColorAttachments = color_attachment_references.begin(),
                .pResolveAttachments = &resolve_attachment_reference,
                .pDepthStencilAttachment = &depth_attachment_reference,
            },
        };
        auto subpass_dependencies = {
            VkSubpassDependency{
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask =
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .dstStageMask =
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = 0,
                .dstAccessMask =
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
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
            }, {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = v.fragment_shader_module.get(),
                .pName = "main",
            }
        };
        VkVertexInputBindingDescription vertex_input_binding_description[]{
            {
                .binding = 0,
                .stride = 4 * 3,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
            {
                .binding = 1,
                .stride = 4 * 3,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
            {
                .binding = 2,
                .stride = 4 * 2,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
        };
        VkVertexInputAttributeDescription vertex_input_attribute_description[]{
            VkVertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0,
            },
            VkVertexInputAttributeDescription{
                .location = 1,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0,
            },
            VkVertexInputAttributeDescription{
                .location = 2,
                .binding = 2,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = 0,
            },
        };
        VkPipelineVertexInputStateCreateInfo input_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount =
                std::size(vertex_input_binding_description),
            .pVertexBindingDescriptions = vertex_input_binding_description,
            .vertexAttributeDescriptionCount =
                std::size(vertex_input_attribute_description),
            .pVertexAttributeDescriptions = vertex_input_attribute_description,
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
            .rasterizationSamples = VK_SAMPLE_COUNT_2_BIT,
            .sampleShadingEnable = VK_FALSE,
            .alphaToCoverageEnable = VK_TRUE,
            .alphaToOneEnable = VK_TRUE,
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

    {
        for (auto i = 0u; i < image_count; i++) {
            images[i].descriptor_sets =
                std::make_unique<VkDescriptorSet[]>(descriptor_set_count);
        }

        auto descriptor_set_layouts = std::make_unique<VkDescriptorSetLayout[]>(
            image_count * descriptor_set_count
        );
        auto descriptor_sets = std::make_unique<VkDescriptorSet[]>(
            image_count * descriptor_set_count
        );

        for (auto i = 0u; i < image_count * descriptor_set_count; i++) {
            descriptor_set_layouts[i] = v.descriptor_set_layout.get();
        }

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descriptor_pool.get(),
            .descriptorSetCount = image_count * descriptor_set_count,
            .pSetLayouts = descriptor_set_layouts.get(),
        };
        check(vkAllocateDescriptorSets(
            v.device.get(), &descriptor_set_allocate_info, descriptor_sets.get()
        ));

        // TODO: double buffer
        auto write_descriptor_sets = std::make_unique<VkWriteDescriptorSet[]>(
            image_count * descriptor_set_count * 2
        );

        // TODO: bind other images
        auto buffer_info = std::make_unique<VkDescriptorBufferInfo[]>(
            descriptor_set_count
        );
        VkDescriptorImageInfo image_info{
            .sampler = v.default_sampler.get(),
            .imageView = v.images[0].view.get(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        for (auto j = 0u; j < descriptor_set_count; j++) {
            buffer_info[j] = {
                .buffer = v.parameter_buffer.get(),
                .offset = sizeof(parameter) * j,
                .range = sizeof(parameter)
            };
        }

        for (auto i = 0u; i < image_count * descriptor_set_count;) {
            for (auto j = 0u; j < descriptor_set_count; j++, i++) {
                write_descriptor_sets[i * 2] = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = buffer_info.get() + j,
                };
                write_descriptor_sets[i * 2 + 1] = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &image_info,
                };
            }
        }

        vkUpdateDescriptorSets(
            v.device.get(), image_count * descriptor_set_count * 2,
            write_descriptor_sets.get(), 0, nullptr
        );

        for (auto i = 0u, s = 0u; i < image_count; i++) {
            for (auto j = 0u; j < descriptor_set_count; j++, s++) {
                images[i].descriptor_sets[j] = descriptor_sets[s];
            }
        }
    }


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
            VkImageCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = surface_format.format,
                .extent = {
                    .width = surface_extent.width,
                    .height = surface_extent.height,
                    .depth = 1
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_2_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage =
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            check(vkCreateImage(
                v.device.get(), &create_info, nullptr,
                out_ptr(image.color_image)
            ));

            VkMemoryRequirements memory_requirements;
            vkGetImageMemoryRequirements(
                v.device.get(), image.color_image.get(), &memory_requirements
            );

            uint32_t color_memory_type_index = 0;
            for (uint32_t i = 0; i < v.properties.memoryTypeCount; i++) {
                if (
                    (
                        v.properties.memoryTypes[i].propertyFlags &
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                        ) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &&
                    (1 << i) & memory_requirements.memoryTypeBits
                ) {
                    color_memory_type_index = i;
                    break;
                }
            }

            VkMemoryAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memory_requirements.size,
                .memoryTypeIndex = color_memory_type_index,
            };

            check(vkAllocateMemory(
                v.device.get(), &allocate_info, nullptr,
                out_ptr(image.color_memory)
            ));

            check(vkBindImageMemory(
                v.device.get(), image.color_image.get(),
                image.color_memory.get(), 0
            ));
        }

        {
            VkImageViewCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = image.color_image.get(),
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
                out_ptr(image.color_view)
            ));
        }

        {
            VkImageCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_D24_UNORM_S8_UINT,
                .extent = {
                    .width = surface_extent.width,
                    .height = surface_extent.height,
                    .depth = 1
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_2_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            check(vkCreateImage(
                v.device.get(), &create_info, nullptr,
                out_ptr(image.depth_image)
            ));

            VkMemoryRequirements memory_requirements;
            vkGetImageMemoryRequirements(
                v.device.get(), image.depth_image.get(), &memory_requirements
            );

            uint32_t depth_memory_type_index = 0;
            for (uint32_t i = 0; i < v.properties.memoryTypeCount; i++) {
                if (
                    (
                        v.properties.memoryTypes[i].propertyFlags &
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                    ) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &&
                    (1 << i) & memory_requirements.memoryTypeBits
                ) {
                    depth_memory_type_index = i;
                    break;
                }
            }

            VkMemoryAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memory_requirements.size,
                .memoryTypeIndex = depth_memory_type_index,
            };

            check(vkAllocateMemory(
                v.device.get(), &allocate_info, nullptr,
                out_ptr(image.depth_memory)
            ));

            check(vkBindImageMemory(
                v.device.get(), image.depth_image.get(),
                image.depth_memory.get(), 0
            ));
        }

        {
            VkImageViewCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = image.depth_image.get(),
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_D24_UNORM_S8_UINT,
                .subresourceRange = {
                    .aspectMask =
                        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };
            check(vkCreateImageView(
                v.device.get(), &create_info, nullptr,
                out_ptr(image.depth_view)
            ));
        }

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
                image.color_view.get(),
                image.depth_view.get(),
                image.image_view.get(),
            };
            VkFramebufferCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = render_pass.get(),
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
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
            .commandPool = v.command_pool.get(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        check(vkAllocateCommandBuffers(
            v.device.get(), &command_buffer_info, &image.draw_command_buffer
        ));

        record_command_buffer(
            c, v, *this, image,
            v.pipeline_layout.get()
        );
    }
}

VkResult view::draw(visuals &v, ::client& client) {
    scope_trace trace;
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

    auto& image = images[image_index];

    VkFence fences[] = {image.draw_finished_fence.get()};
    
    {
        scope_trace trace;
        check(vkWaitForFences(
            v.device.get(), 1, fences,
            VK_TRUE, ~0ul
        ));
    }
    check(vkResetFences(
        v.device.get(), 1, fences
    ));

    {
        scope_trace trace;
        if (client.update_number != image.update_number) {
            check(vkResetCommandBuffer(image.draw_command_buffer, 0));
            record_command_buffer(
                client, v, *v.view, image, v.pipeline_layout.get()
            );
            image.update_number = client.update_number;
        }

        glm::mat4 projection = glm::infinitePerspective(
            glm::radians(60.0f),
            (float)surface_extent.width / surface_extent.height,
            0.01f
        );

        glm::mat4 view = glm::mat4_cast(glm::inverse(client.user_orientation));
        view = glm::translate(view, -client.user_position);


        // TODO: read VkPhysicalDeviceLimits::nonCoherentAtomSize
        uint32_t size = round_up(sizeof(::parameters), 128);

        mapped_allocation parameter_mapping;
        vulkan_memory_allocator_map_memory(
            v.parameter_allocation.get(), out_ptr(parameter_mapping)
        );
        ::parameters* parameters = (::parameters*)parameter_mapping->bytes;
        parameters->parameters[0].model_view_projection_matrix =
            projection * view;

        auto primitive = 0u;
        for (auto j = 0u; j < client.world_model.primitives.size(); j++) {
            if (primitive >= std::size(parameters->parameters))
                break;
            auto model = glm::mat4(glm::mat3(-1, 0, 0, 0, 0, 1, 0, 1, 0));
            parameters->parameters[primitive].model_view_projection_matrix =
                projection * view * model;
            parameters->parameters[primitive].model_matrix = model;
            primitive++;
        }
        for (auto i = 0u; i < client.users.position.size(); i++) {
            for (auto j = 0u; j < client.test_model.primitives.size(); j++) {
                if (primitive >= std::size(parameters->parameters))
                    break;
                auto model = glm::translate(
                        glm::mat4(1.0),
                        glm::vec3(client.users.position[i])
                    ) *
                    glm::mat4_cast(client.users.orientation[i]) *
                    glm::scale(glm::mat4(1.0), {-1, -1, 1}) *
                    glm::translate(
                        glm::mat4(1.0),
                        glm::vec3(0, -1.37, 0.08)
                    );
                parameters->parameters[primitive].model_view_projection_matrix =
                    projection * view * model;
                parameters->parameters[primitive].model_matrix = model;
                primitive++;
            }
        }

        vmaFlushAllocation(
            v.allocator.get(), v.parameter_allocation.get(), 0, VK_WHOLE_SIZE
        );
    }


    VkSemaphore wait_semaphores[] =
        {v.swapchain_image_ready_semaphore.get()};
    VkSemaphore signal_semaphores[] =
        {images[image_index].draw_finished_semaphore.get()};
    VkCommandBuffer buffers[] = {images[image_index].draw_command_buffer};
    VkPipelineStageFlags wait_stage[] =
        {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    VkSubmitInfo submit_info = {
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
        v.graphics_queue, 1, &submit_info,
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
