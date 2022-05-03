cmake_minimum_required(VERSION 3.5)

project(hello-emscripten LANGUAGES CXX)

add_subdirectory(source)

add_subdirectory(submodules/VulkanGL EXCLUDE_FROM_ALL)