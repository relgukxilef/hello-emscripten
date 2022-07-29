cmake_minimum_required(VERSION 3.13)

add_executable(
    hello-emscripten
    hello.h
    hello.cpp
    utility/out_ptr.h
    utility/resource.h
    utility/vulkan_resource.h
    utility/vulkan_resource.cpp
)

target_link_libraries(
    hello-emscripten PRIVATE
    Vulkan-Headers
)

if (EMSCRIPTEN)
    target_sources(
        hello-emscripten PUBLIC
        main-ems.cpp
    )

    target_link_libraries(
        hello-emscripten PRIVATE 
        vulkangl
    )

    set_target_properties(hello-emscripten PROPERTIES SUFFIX ".html")

else()
    target_sources(
        hello-emscripten PUBLIC
        main-vk-glfw.cpp
    )

    target_link_libraries(
        hello-emscripten PRIVATE 
        vulkan-1 glfw3
    )
endif()

target_compile_features(hello-emscripten PUBLIC cxx_std_17)
