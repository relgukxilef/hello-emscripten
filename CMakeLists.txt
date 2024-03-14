cmake_minimum_required(VERSION 3.5)

project(hello-emscripten LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(source)

add_subdirectory(submodules/glm)

add_subdirectory(submodules/SPIRV-Cross EXCLUDE_FROM_ALL)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)

add_subdirectory(submodules/boost EXCLUDE_FROM_ALL)

set(ZLIB_COMPAT ON)
add_subdirectory(submodules/zlib-ng EXCLUDE_FROM_ALL)

set(ZLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/submodules/zlib-ng")
set(SKIP_INSTALL_ALL ON)
add_subdirectory(submodules/libpng EXCLUDE_FROM_ALL)
