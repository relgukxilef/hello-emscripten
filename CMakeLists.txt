cmake_minimum_required(VERSION 3.12)

project(hello-emscripten LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_policy(SET CMP0077 NEW) # option() honors normal variables
cmake_policy(SET CMP0074 NEW) # Enable find_package uses of <>_ROOT variables
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(source)

add_subdirectory(submodules/glm)

add_subdirectory(submodules/SPIRV-Cross EXCLUDE_FROM_ALL)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)

add_subdirectory(submodules/boost EXCLUDE_FROM_ALL)

set(ZLIB_COMPAT ON)
set(ZLIB_ROOT "${CMAKE_CURRENT_BINARY_DIR}/submodules/zlib-ng")
set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/submodules/zlib-ng/zlibstatic")
add_subdirectory(submodules/zlib-ng EXCLUDE_FROM_ALL)

add_subdirectory(submodules/libpng EXCLUDE_FROM_ALL)
