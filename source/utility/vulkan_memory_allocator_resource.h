#pragma once

#include <vk_mem_alloc.h>

#include "resource.h"
#include "vulkan_resource.h"

extern VmaAllocator current_allocator;

inline void vulkan_memory_allocator_delete_allocator(VmaAllocator* allocator) {
    vmaDestroyAllocator(*allocator);
}

inline void vulkan_memory_allocator_delete_allocation(
    VmaAllocation* allocation
) {
    vmaFreeMemory(current_allocator, *allocation);
}

typedef unique_resource<VmaAllocator, vulkan_memory_allocator_delete_allocator>
    unique_allocator;

typedef unique_resource<
    VmaAllocation, vulkan_memory_allocator_delete_allocation
> unique_allocation;
