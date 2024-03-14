cmake_minimum_required(VERSION 3.5)

project(hello-emscripten LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_subdirectory(source)

add_subdirectory(submodules/glm)

add_subdirectory(submodules/SPIRV-Cross EXCLUDE_FROM_ALL)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)

add_subdirectory(submodules/boost EXCLUDE_FROM_ALL)

set(ZLIB_COMPAT ON)
add_subdirectory(submodules/zlib-ng EXCLUDE_FROM_ALL)

find_package(Zlib REQUIRED)

set(SKIP_INSTALL_ALL ON)
add_subdirectory(submodules/libpng EXCLUDE_FROM_ALL)
