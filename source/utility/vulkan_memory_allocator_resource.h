#pragma once

#include <vk_mem_alloc.h>

#include "resource.h"
#include "vulkan_resource.h"

extern VmaAllocator current_allocator;

struct vulkan_memory_allocator_mapping {
    VmaAllocation allocation;
    std::uint8_t *bytes;
    
    bool operator==(
        const vulkan_memory_allocator_mapping&
    ) const = default;
};

inline void vulkan_memory_allocator_delete_allocator(VmaAllocator* allocator) {
    vmaDestroyAllocator(*allocator);
}

inline void vulkan_memory_allocator_delete_allocation(
    VmaAllocation* allocation
) {
    vmaFreeMemory(current_allocator, *allocation);
}

inline void vulkan_memory_allocator_map_memory(
    VmaAllocation allocation,
    vulkan_memory_allocator_mapping* mapping
) {
    check(vmaMapMemory(
        current_allocator, allocation, (void**)&mapping->bytes
    ));
    mapping->allocation = allocation;
}

inline void vulkan_memory_allocator_unmap_memory(
    vulkan_memory_allocator_mapping* mapping
) {
    vmaUnmapMemory(
        current_allocator, mapping->allocation
    );
}

typedef unique_resource<VmaAllocator, vulkan_memory_allocator_delete_allocator>
    unique_allocator;

typedef unique_resource<
    VmaAllocation, vulkan_memory_allocator_delete_allocation
> unique_allocation;

typedef unique_resource<
    vulkan_memory_allocator_mapping, vulkan_memory_allocator_unmap_memory
> mapped_allocation;
