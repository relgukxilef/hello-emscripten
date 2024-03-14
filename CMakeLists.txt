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
add_library(ZLIB::ZLIB ALIAS zlibstatic)

set(ZLIB_ROOT submodules/zlib-ng)
set(SKIP_INSTALL_ALL ON)
# libpng expects ZLIB_INCLUDE_DIRS, but zlib-ng only sets ZLIB_INCLUDE_DIR
set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
add_subdirectory(submodules/libpng EXCLUDE_FROM_ALL)
