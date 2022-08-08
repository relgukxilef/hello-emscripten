cmake_minimum_required(VERSION 3.13)

add_library(
    hello STATIC
    hello.h hello.cpp
    utility/out_ptr.h
    utility/resource.h
    utility/vulkan_resource.h utility/vulkan_resource.cpp
    visuals/visuals.h visuals/visuals.cpp
    visuals/view.h visuals/view.cpp
)

target_link_libraries(
    hello PRIVATE
    Vulkan-Headers
)

add_executable(
    hello-vk
    main-vk-glfw.cpp
)

target_link_libraries(
    hello-vk PRIVATE
    hello
    vulkan-1 glfw3
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
    add_executable(
        hello-gl
        main-gl-glfw.cpp
    )

    target_link_libraries(
        hello-gl PUBLIC
        glfw3 glew32s opengl32
    )

    target_compile_definitions(hello-gl PRIVATE GLEW_STATIC)
endif()

target_link_libraries(
    hello-gl PRIVATE
    hello
    vulkangl
)

target_compile_features(hello PUBLIC cxx_std_20)
target_compile_features(hello-gl PUBLIC cxx_std_20)
