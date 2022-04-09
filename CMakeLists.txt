cmake_minimum_required(VERSION 3.5)

project(hello-emscripten LANGUAGES CXX)

add_subdirectory(source)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)

target_compile_options(hello-emscripten PUBLIC -std=c++17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)