cmake_minimum_required(VERSION 3.13)

project(hello-emscripten LANGUAGES CXX C ASM)

set(VERBOSE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # option() honors normal variables
cmake_policy(SET CMP0074 NEW) # Enable find_package uses of <>_ROOT variables
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_SIZEOF_VOID_P)
    # Set a default value for CMAKE_SIZEOF_VOID_P
    # It is not set when building with emscripten
    set(CMAKE_SIZEOF_VOID_P 4)
endif()

if (EMSCRIPTEN)
    # Needed for boost
    add_compile_options(-fdeclspec)
endif()

add_subdirectory(submodules/glm EXCLUDE_FROM_ALL)

add_subdirectory(submodules/SPIRV-Cross EXCLUDE_FROM_ALL)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)

set(BOOST_IOSTREAMS_ENABLE_ZLIB OFF)
add_subdirectory(submodules/boost EXCLUDE_FROM_ALL)

target_compile_definitions(
    boost_log PUBLIC _WIN32_WINNT=0x0A00 BOOST_USE_WINAPI_VERSION=0x0A00
)

if (NOT EMSCRIPTEN)
    # Can't figure out how to keep libpng from using the system library for
    # generated sources
    set(ZLIB_COMPAT ON)
    add_subdirectory(submodules/zlib-ng EXCLUDE_FROM_ALL)
    add_library(ZLIB::ZLIB ALIAS zlib)

    set(ZLIB_ROOT "${CMAKE_CURRENT_BINARY_DIR}/submodules/zlib-ng")
    set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/submodules/zlib-ng/zlib")
    set(ZLIBINC "${ZLIB_ROOT}")
    set(ZLIB_INCLUDE_DIR "${ZLIB_ROOT}")

    set(AWK "")

    add_subdirectory(submodules/libpng EXCLUDE_FROM_ALL)
    target_include_directories(
        png_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/submodules/libpng>
    )
endif()

if (EMSCRIPTEN)
    set(OPUS_STACK_PROTECTOR OFF)
    set(OPUS_FORTIFY_SOURCE OFF)
endif()
add_subdirectory(submodules/opus EXCLUDE_FROM_ALL)

if (NOT EMSCRIPTEN)
    set(ALSOFT_UTILS OFF)
    add_subdirectory(submodules/openal-soft EXCLUDE_FROM_ALL)
endif()

if (NOT EMSCRIPTEN)
    set(GLFW_BUILD_DOCS OFF)
    add_subdirectory(submodules/glfw EXCLUDE_FROM_ALL)
else()
    set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
    set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON)
    set(Vulkan_LIBRARY vulkangl)
    get_target_property(
        Vulkan_INCLUDE_DIR Vulkan-Headers INTERFACE_INCLUDE_DIRECTORIES
    )
endif()

add_subdirectory(submodules/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

include(ExternalProject)

ExternalProject_Add(
    OpenSSL
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/submodules/openssl
    CONFIGURE_COMMAND ./config no-asm no-shared --prefix=${CMAKE_BINARY_DIR}/openssl
    BUILD_COMMAND make
    INSTALL_COMMAND make install_sw
)

add_subdirectory(source)
