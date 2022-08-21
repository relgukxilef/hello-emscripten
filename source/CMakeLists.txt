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

    set_target_properties(hello-gl PROPERTIES SUFFIX ".js")
    set_target_properties(hello-gl PROPERTIES OUTPUT_NAME "index")

    add_custom_command(
        TARGET hello-gl POST_BUILD
        COMMAND 
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/html/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
    )

    if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
        target_compile_options(hello-gl PRIVATE -fsanitize=address)
        target_link_options(hello-gl PRIVATE -fsanitize=address)
    endif ()

    target_link_options(
        hello-gl PRIVATE
        -sNO_EXIT_RUNTIME
        -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
    )

    target_link_options(
        hello-gl PUBLIC
        --preload-file ${CMAKE_CURRENT_BINARY_DIR}/visuals@visuals
        -sINITIAL_MEMORY=268435456
    )

else()
    find_package(glfw3 3.3 REQUIRED)
    find_package(Vulkan)
    find_package(OpenGL)

    target_link_libraries(
        hello PRIVATE
        glfw ${CMAKE_DL_LIBS}
    )

    add_executable(
        hello-vk
        main-vk-glfw.cpp
    )

    add_executable(
        hello-gl
        main-gl-glfw.cpp
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

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
