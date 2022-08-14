cmake_minimum_required(VERSION 3.13)

include(utility.cmake)

add_library(
    hello STATIC
    hello.h hello.cpp
    utility/out_ptr.h
    utility/resource.h
    utility/vulkan_resource.h utility/vulkan_resource.cpp
    visuals/visuals.h visuals/visuals.cpp
    visuals/view.h visuals/view.cpp
)

target_shader(hello visuals/shaders/solid_vertex.glsl)
target_shader(hello visuals/shaders/solid_fragment.glsl)

target_link_libraries(
    hello PRIVATE
    Vulkan-Headers
)

if (EMSCRIPTEN)
    add_executable(
        hello-gl
        main-em.cpp
    )

    set_target_properties(hello-gl PROPERTIES SUFFIX ".html")
    set_target_properties(hello-gl PROPERTIES OUTPUT_NAME "index")
    target_compile_options(hello-gl PRIVATE -fsanitize=address)
    target_link_options(
        hello-gl PRIVATE
        -fsanitize=address -sNO_EXIT_RUNTIME
        -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
    )

else()
    find_package(glfw3 3.3 REQUIRED)
    find_package(Vulkan)
    find_package(OpenGL)

    target_link_libraries(
        hello PRIVATE
        glfw
    )

    add_executable(
        hello-vk
        main-vk-glfw.cpp
    )

    add_executable(
        hello-gl
        main-gl-glfw.cpp
        egl.c gles2.c
    )

    target_link_libraries(
        hello-gl PUBLIC
        OpenGL::GL
    )

    target_link_libraries(
        hello-vk PUBLIC
        hello
        Vulkan::Vulkan
    )
endif()

target_link_libraries(
    hello-gl PRIVATE
    hello
    vulkangl
)

target_compile_features(hello PUBLIC cxx_std_20)
