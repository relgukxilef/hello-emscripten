#pragma once

#include "vulkan/vulkan_core.h"

#include "../utility/resource.h"

struct indirect_draw_buffer;

indirect_draw_buffer* make_indirect_draw_buffer(int size);
void delete_indirect_draw_buffer(indirect_draw_buffer*);

typedef unique_resource<
    indirect_draw_buffer*, delete_indirect_draw_buffer
> unique_indirect_draw_buffer;

void map_memory(indirect_draw_buffer*);

void set_commands(
    indirect_draw_buffer*, int index, int count,
    VkDrawIndirectCommand* commands
);

void unmap_memory(indirect_draw_buffer*);

void indirect_draw(
    VkCommandBuffer commandBuffer,
    indirect_draw_buffer* buffer,
    VkDeviceSize offset,
    uint32_t drawCount
);
