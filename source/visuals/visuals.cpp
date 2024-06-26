#include "visuals.h"

#include <cstring>
#include <memory>
#include <cstdio>
#include <array>
#include <vector>

#include "../utility/out_ptr.h"
#include "../utility/file.h"
#include "../utility/math.h"
#include "../utility/trace.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*
) noexcept {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::fprintf(
            stderr, "Validation layer error: %s\n", callback_data->pMessage
        );
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::fprintf(
            stderr, "Validation layer warning: %s\n", callback_data->pMessage
        );
    }

    return VK_FALSE;
}

visuals::visuals(::client& client, VkInstance instance, VkSurfaceKHR surface) {
    // create debug utils messenger
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = nullptr
    };
    auto vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT"
        );
    {
        check(vkCreateDebugUtilsMessengerEXT(
            instance, &debug_utils_messenger_create_info, nullptr,
            out_ptr(debug_utils_messenger)
        ));
    }

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    {
        auto devices = std::make_unique<VkPhysicalDevice[]>(device_count);
        check(vkEnumeratePhysicalDevices(
            instance, &device_count, devices.get()
        ));

        physical_device = devices[0]; // just pick the first one
    }

    // find memory types
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, nullptr
    );
    auto queue_families =
        std::make_unique<VkQueueFamilyProperties[]>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, queue_families.get()
    );

    for (auto i = 0u; i < queue_family_count; i++) {
        const auto& queueFamily = queue_families[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            physical_device, i, surface, &present_support
        );
        if (present_support) {
            present_queue_family = i;
        }
    }
    if (graphics_queue_family == ~0u) {
        throw std::runtime_error("no suitable queue found");
    }

    // create logical device
    {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_infos[]{
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphics_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }, {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = present_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }
        };

        const char* enabled_extension_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceFeatures device_features{
            .alphaToOne = VK_TRUE,
        };
        VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount =
                static_cast<uint32_t>(std::size(queue_create_infos)),
            .pQueueCreateInfos = queue_create_infos,
            .enabledExtensionCount =
                static_cast<uint32_t>(std::size(enabled_extension_names)),
            .ppEnabledExtensionNames = enabled_extension_names,
            .pEnabledFeatures = &device_features,
        };

        check(vkCreateDevice(
            physical_device, &create_info, nullptr, out_ptr(device)
        ));
    }
    current_device = device.get();

    // retreive queues
    vkGetDeviceQueue(device.get(), graphics_queue_family, 0, &graphics_queue);
    vkGetDeviceQueue(device.get(), present_queue_family, 0, &present_queue);

    {
        VkSemaphoreCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        check(vkCreateSemaphore(
            device.get(), &create_info, nullptr,
            out_ptr(swapchain_image_ready_semaphore)
        ));
    }

    // create shader modules
    {
        auto code = read_file("visuals/shaders/solid_vertex.glsl.spv");
        VkShaderModuleCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = (size_t)(code.size()),
            .pCode = reinterpret_cast<uint32_t*>(code.data()),
        };
        check(vkCreateShaderModule(
            device.get(), &create_info, nullptr, out_ptr(vertex_shader_module)
        ));
    }
    {
        auto code = read_file("visuals/shaders/solid_fragment.glsl.spv");
        VkShaderModuleCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = (size_t)(code.size()),
            .pCode = reinterpret_cast<uint32_t*>(code.data()),
        };
        check(vkCreateShaderModule(
            device.get(), &create_info, nullptr, out_ptr(fragment_shader_module)
        ));
    }

    // create descriptor set
    // TODO: move to command buffer recording to allow resizing
    {
        auto descriptor_set_layout_binding = {
            VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
        };
        VkDescriptorSetLayoutCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount =
                static_cast<uint32_t>(descriptor_set_layout_binding.size()),
            .pBindings = descriptor_set_layout_binding.begin(),
        };
        check(vkCreateDescriptorSetLayout(
            device.get(), &create_info,
            nullptr, out_ptr(descriptor_set_layout)
        ));
    }

    // create pipeline layouts
    {
        VkDescriptorSetLayout set_layouts[] {
            descriptor_set_layout.get(),
        };

        VkPipelineLayoutCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = std::size(set_layouts),
            .pSetLayouts = set_layouts
        };
        check(vkCreatePipelineLayout(
            device.get(), &create_info, nullptr, out_ptr(pipeline_layout)
        ));
    }

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
        if (
            (
                properties.memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            ) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        ) {
            host_visible_memory_type_index = i;
            break;
        }
    }

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
        if (
            (
                properties.memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            ) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        ) {
            device_local_memory_type_index = i;
            break;
        }
    }

    {
        VkCommandPoolCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics_queue_family,
        };
        check(vkCreateCommandPool(
            device.get(), &create_info, nullptr, out_ptr(command_pool)
        ));
    }

    // create buffers
    {
        VkBufferCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = memory_size,
            .usage =
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        check(vkCreateBuffer(
            device.get(), &create_info, nullptr, out_ptr(host_visible_buffer)
        ));
    }

    {
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(
            device.get(), host_visible_buffer.get(), &requirements
        );

        if (
            ~requirements.memoryTypeBits & (1 << host_visible_memory_type_index)
        ) {
            // TODO: do this check when looking for memory types
            throw;
        }

        VkMemoryAllocateInfo allocate_info {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = host_visible_memory_type_index,
        };

        check(vkAllocateMemory(
            device.get(), &allocate_info, nullptr, out_ptr(host_visible_memory)
        ));

        check(vkBindBufferMemory(
            device.get(), host_visible_buffer.get(),
            host_visible_memory.get(), 0
        ));
    }

    unsigned memory_offset = 0;
    view_parameters_offset = memory_offset;
    memory_offset += sizeof(parameters);
    user_position_offset = memory_offset;
    memory_offset += sizeof(glm::vec4) * 16;

    memory_offset = round_up(memory_offset, 128);

    uint8_t *memory;
    check(vkMapMemory(
        device.get(), host_visible_memory.get(), 0, memory_size, 0,
        (void**)&memory
    ));

    uint8_t *i = memory + memory_offset;

    {
        VkSamplerCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        };

        check(vkCreateSampler(
            device.get(), &create_info, nullptr, out_ptr(default_sampler)
        ));
    }

    VkMemoryAllocateInfo allocate_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_size,
        .memoryTypeIndex = device_local_memory_type_index,
    };

    check(vkAllocateMemory(
        device.get(), &allocate_info, nullptr,
        out_ptr(device_local_memory)
    ));

    uint32_t image_begin = 0;

    for (auto &model : {client.test_model, client.world_model}) {
        models.push_back({});
        auto& visual_model = models.back();
        visual_model.position_offset = i - memory;
        i = std::ranges::copy(model.positions, i).out;
        visual_model.normal_offset = i - memory;
        i = std::ranges::copy(model.normals, i).out;
        visual_model.texture_coordinate_offset = i - memory;
        i = std::ranges::copy(model.texture_coordinates, i).out;
        visual_model.indices_offset = i - memory;
        i = std::ranges::copy(model.indices, i).out;
        visual_model.images_offset = i - memory;
        i = std::ranges::copy(model.pixels, i).out;

        for (auto image : model.images) {
            model_images.push_back({});
            VkImageCreateInfo create_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .extent = {image.width, image.height, 1},
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage =
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            check(vkCreateImage(
                device.get(), &create_info, nullptr,
                out_ptr(model_images.back())
            ));

            {
                check(vkBindImageMemory(
                    device.get(), model_images.back().get(),
                    device_local_memory.get(), image_begin
                ));

                model_image_views.push_back({});
                VkImageViewCreateInfo create_info{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = model_images.back().get(),
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = VK_FORMAT_R8G8B8A8_SRGB,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                check(vkCreateImageView(
                    device.get(), &create_info, nullptr,
                    out_ptr(model_image_views.back())
                ));
            }

            image_begin = round_up(image_begin + image.size, 1024);

            {
                VkCommandBuffer copy_command;
                VkCommandBufferAllocateInfo allocate_info {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .commandPool = command_pool.get(),
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1,
                };
                check(vkAllocateCommandBuffers(
                    device.get(), &allocate_info, &copy_command
                ));

                VkCommandBufferBeginInfo begin_info = {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                };
                check(vkBeginCommandBuffer(copy_command, &begin_info));

                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = model_images.back().get(),
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };
                vkCmdPipelineBarrier(
                    copy_command,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                VkBufferImageCopy buffer_image_copy = {
                    .bufferOffset = visual_model.images_offset + image.begin,
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,
                    .imageSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .imageOffset = {0, 0, 0},
                    .imageExtent = {
                        image.width, image.height, 1
                    },
                };
                vkCmdCopyBufferToImage(
                    copy_command, host_visible_buffer.get(),
                    model_images.back().get(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &buffer_image_copy
                );

                barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = model_images.back().get(),
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };
                vkCmdPipelineBarrier(
                    copy_command,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                check(vkEndCommandBuffer(copy_command));

                VkSubmitInfo submit_info = {
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .commandBufferCount = 1,
                    .pCommandBuffers = &copy_command,
                };
                check(vkQueueSubmit(
                    graphics_queue, 1, &submit_info, VK_NULL_HANDLE
                ));
                check(vkQueueWaitIdle(graphics_queue));
            }
        }
    }

    VkMappedMemoryRange ranges[] = {
        VkMappedMemoryRange{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory = host_visible_memory.get(),
            .offset = 0,
            .size = static_cast<VkDeviceSize>(memory_size),
        }
    };
    check(vkFlushMappedMemoryRanges(device.get(), 1, ranges));
    vkUnmapMemory(device.get(), host_visible_memory.get());

    view.reset(new ::view(client, *this, instance, surface));
}

void visuals::draw(
    ::client& client, VkInstance instance, VkSurfaceKHR surface
) {
    scope_trace trace;
    if (view) {
        if (view->draw(*this, client) != VK_SUCCESS) {
            view.reset(); // delete first
            view.reset(new ::view(client, *this, instance, surface));
        }
    }
}
