#include "visuals.h"

#include <cstring>
#include <memory>
#include <cstdio>
#include <array>
#include <vector>

#include "../utility/out_ptr.h"
#include "../utility/file.h"
#include "../utility/math.h"

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

        VkPhysicalDeviceFeatures device_features{};
        VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount =
                static_cast<uint32_t>(std::size(queue_create_infos)),
            .pQueueCreateInfos = queue_create_infos,
            .enabledExtensionCount =
                static_cast<uint32_t>(std::size(enabled_extension_names)),
            .ppEnabledExtensionNames = enabled_extension_names,
            .pEnabledFeatures = &device_features
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

    // find memory types
    {
        VkPhysicalDeviceMemoryProperties properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);

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
    model_position_offset = memory_offset;

    memory_offset = round_up(model_position_offset, 128);

    {
        uint8_t *memory;
        check(vkMapMemory(
            device.get(), host_visible_memory.get(), 0, memory_size, 0,
            (void**)&memory
        ));

        uint8_t *i = memory + memory_offset;

        i = std::ranges::copy(client.test_model.positions, i).out;
        model_indices_offset = i - memory;
        i = std::ranges::copy(client.test_model.indices, i).out;

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
    }

    view.reset(new ::view(client, *this, instance, surface));
}

void visuals::draw(
    ::client& client, VkInstance instance, VkSurfaceKHR surface
) {
    if (view) {
        if (view->draw(*this, client) != VK_SUCCESS) {
            view.reset(); // delete first
            view.reset(new ::view(client, *this, instance, surface));
        }
    }
}
