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

visuals::visuals(
    ::client& client, VkInstance instance, VkSurfaceKHR surface, 
    VkPhysicalDevice physical_device, VkDevice device, 
    XrInstance xr_instance, XrSystemId system_id, XrSession session,
    VkPhysicalDeviceMemoryProperties properties,
    uint32_t graphics_queue_family, uint32_t present_queue_family
) {
    this->instance = instance;
    this->surface = surface;
    this->physical_device = physical_device;
    this->device = device;
    this->xr_instance = xr_instance;
    this->system_id = system_id;
    this->session = session;
    this->properties = properties;
    this->graphics_queue_family = graphics_queue_family;
    this->present_queue_family = present_queue_family;

    {
        VmaVulkanFunctions vulkanFunctions = {
            .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = &vkGetDeviceProcAddr,
        };

        VmaAllocatorCreateInfo allocatorCreateInfo = {
            .physicalDevice = physical_device,
            .device = device,
            .pVulkanFunctions = &vulkanFunctions,
            .instance = instance,
            .vulkanApiVersion = VK_API_VERSION_1_0,
        };

        vmaCreateAllocator(&allocatorCreateInfo, out_ptr(allocator));
    }
    current_allocator = allocator.get();

    // retreive queues
    vkGetDeviceQueue(device, graphics_queue_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_queue_family, 0, &present_queue);

    {
        VkSemaphoreCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        check(vkCreateSemaphore(
            device, &create_info, nullptr,
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
            device, &create_info, nullptr, out_ptr(vertex_shader_module)
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
            device, &create_info, nullptr, out_ptr(fragment_shader_module)
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
            device, &create_info,
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
            device, &create_info, nullptr, out_ptr(pipeline_layout)
        ));
    }

    {
        VkCommandPoolCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics_queue_family,
        };
        check(vkCreateCommandPool(
            device, &create_info, nullptr, out_ptr(command_pool)
        ));
    }

    // create buffers
    {
        VkBufferCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(parameters),
            .usage =
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VmaAllocationCreateInfo allocation_create_info {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        
        check(vmaCreateBuffer(
            allocator.get(), &create_info, &allocation_create_info, 
            out_ptr(parameter_buffer), out_ptr(parameter_allocation), nullptr
        ));
    }

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
            device, &create_info, nullptr, out_ptr(default_sampler)
        ));
    }

    mapped_allocation vertex_mapping, index_mapping, pixel_mapping;
    {
        VkBufferCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = vertex_memory_size,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VmaAllocationCreateInfo allocation_create_info {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        check(vmaCreateBuffer(
            allocator.get(), &create_info, &allocation_create_info, 
            out_ptr(vertex_buffer), 
            out_ptr(vertex_allocation), nullptr
        ));
        vulkan_memory_allocator_map_memory(
            vertex_allocation.get(), out_ptr(vertex_mapping)
        );
    }
    {
        VkBufferCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = index_memory_size,
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VmaAllocationCreateInfo allocation_create_info {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        VmaAllocationInfo allocation_info;
        check(vmaCreateBuffer(
            allocator.get(), &create_info, &allocation_create_info, 
            out_ptr(index_buffer), 
            out_ptr(index_allocation), 
            &allocation_info
        ));
        vulkan_memory_allocator_map_memory(
            index_allocation.get(), out_ptr(index_mapping)
        );
    }
    {
        VkBufferCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = pixel_memory_size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VmaAllocationCreateInfo allocation_create_info {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        VmaAllocationInfo allocation_info;
        check(vmaCreateBuffer(
            allocator.get(), &create_info, &allocation_create_info, 
            out_ptr(pixel_buffer), 
            out_ptr(pixel_allocation), 
            &allocation_info
        ));
        vulkan_memory_allocator_map_memory(
            pixel_allocation.get(), out_ptr(pixel_mapping)
        );
    }

    uint8_t 
        *vertices = vertex_mapping->bytes, 
        *indices = index_mapping->bytes, 
        *pixels = pixel_mapping->bytes;
    uint8_t *vertex = vertices, *index = indices, *pixel = pixels;

    for (auto &model : {client.test_model, client.world_model}) {
        models.push_back({});
        auto& visual_model = models.back();

        visual_model.position_offset = vertex - vertices;
        vertex = std::ranges::copy(model.positions, vertex).out;
        visual_model.normal_offset = vertex - vertices;
        vertex = std::ranges::copy(model.normals, vertex).out;
        visual_model.texture_coordinate_offset = vertex - vertices;
        vertex = std::ranges::copy(model.texture_coordinates, vertex).out;

        visual_model.indices_offset = index - indices;
        index = std::ranges::copy(model.indices, index).out;

        visual_model.images_offset = pixel - pixels;
        pixel = std::ranges::copy(model.pixels, pixel).out;

        check(vmaFlushAllocation(
            allocator.get(), pixel_allocation.get(), 
            visual_model.images_offset, model.pixels.size()
        ));

        for (auto image : model.images) {
            if (image.width == 0 || image.height == 0) {
                image.width = 16;
                image.height = 16;
            }
            images.push_back({});
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
            VmaAllocationCreateInfo allocation_info{
                .usage = VMA_MEMORY_USAGE_AUTO,
            };
            check(vmaCreateImage(
                allocator.get(), &create_info, &allocation_info, 
                out_ptr(images.back().image), 
                out_ptr(images.back().allocation),
                nullptr
            ));

            {
                VkImageViewCreateInfo create_info{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = images.back().image.get(),
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
                    device, &create_info, nullptr,
                    out_ptr(images.back().view)
                ));
            }

            {
                VkCommandBuffer copy_command;
                VkCommandBufferAllocateInfo allocate_info {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .commandPool = command_pool.get(),
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1,
                };
                check(vkAllocateCommandBuffers(
                    device, &allocate_info, &copy_command
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
                    .image = images.back().image.get(),
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
                    copy_command, pixel_buffer.get(),
                    images.back().image.get(),
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
                    .image = images.back().image.get(),
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
                check(vkQueueWaitIdle(graphics_queue)); // TODO: remove this
            }
        }
    }

    VmaAllocation allocations[] = 
        { vertex_allocation.get(), index_allocation.get(), };
    check(vmaFlushAllocations(
        allocator.get(), std::size(allocations),
        allocations, nullptr, nullptr
    ));

    view.reset(new ::view(client, *this));
}

void visuals::draw(::client& client) {
    scope_trace trace;

    if (view->draw(*this, client) != VK_SUCCESS) {
        view.reset(); // delete first
        view.reset(new ::view(client, *this));
    }
}
