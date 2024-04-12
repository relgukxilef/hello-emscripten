cmake_minimum_required(VERSION 3.13)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(hello-emscripten LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_policy(SET CMP0077 NEW) # option() honors normal variables
cmake_policy(SET CMP0074 NEW) # Enable find_package uses of <>_ROOT variables
set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_SIZEOF_VOID_P)
    # Set a default value for CMAKE_SIZEOF_VOID_P
    # It is not set when building with emscripten
    set(CMAKE_SIZEOF_VOID_P 4)  # Example: Set to 4 for 32-bit pointers
endif()

if (EMSCRIPTEN)
    # Needed for boost
    add_compile_options(-fdeclspec)
endif()

add_subdirectory(source)

add_subdirectory(submodules/glm)

add_subdirectory(submodules/SPIRV-Cross EXCLUDE_FROM_ALL)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)

add_subdirectory(submodules/boost EXCLUDE_FROM_ALL)

set(ZLIB_COMPAT ON)
set(ZLIB_ROOT "${CMAKE_CURRENT_BINARY_DIR}/submodules/zlib-ng")
set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/submodules/zlib-ng/zlibstatic")
unset(BUILD_SHARED_LIBS)
unset(BUILD_SHARED_LIBS CACHE)
add_subdirectory(submodules/zlib-ng EXCLUDE_FROM_ALL)
add_library(ZLIB::ZLIB ALIAS zlibstatic)

add_subdirectory(submodules/libpng EXCLUDE_FROM_ALL)

set(OPUS_STACK_PROTECTOR OFF)
add_subdirectory(submodules/opus EXCLUDE_FROM_ALL)

add_subdirectory(submodules/openal-soft EXCLUDE_FROM_ALL)

if (NOT EMSCRIPTEN)
    set(GLFW_BUILD_DOCS OFF)
    add_subdirectory(submodules/glfw EXCLUDE_FROM_ALL)
endif()
