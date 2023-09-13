cmake_minimum_required(VERSION 3.13)

project(hello-emscripten LANGUAGES CXX C)

add_subdirectory(source)

add_subdirectory(submodules/glm)

add_subdirectory(submodules/SPIRV-Cross EXCLUDE_FROM_ALL)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)

set(OPUS_STACK_PROTECTOR OFF)
add_subdirectory(submodules/opus EXCLUDE_FROM_ALL)
