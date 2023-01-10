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
    state/input.h state/input.cpp
    state/client.h state/client.cpp
    network/network_client.h network/network_client.cpp
    network/websocket.h
    visuals/indirect_draw.h
    network/network_message.h network/network_message.cpp
)

target_shader(hello visuals/shaders/solid_vertex.glsl)
target_shader(hello visuals/shaders/solid_fragment.glsl)

target_link_libraries(
    hello PUBLIC
    Vulkan-Headers
    glm
)

if (EMSCRIPTEN)
    add_executable(
        hello-gl
        main-em.cpp
        network/websocket-em.cpp
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
        -lwebsocket.js
    )

    target_link_options(
        hello-gl PUBLIC
        --preload-file ${CMAKE_CURRENT_BINARY_DIR}/visuals@visuals
        -sINITIAL_MEMORY=268435456
    )

else()
    find_package(glfw3 3.3)
    find_package(Vulkan)
    find_package(OpenGL)
    find_package(Boost COMPONENTS system REQUIRED)    

    target_sources(
        hello PRIVATE
        network/websocket-boost.cpp
    )

    target_link_libraries(
        hello PRIVATE
        ${CMAKE_DL_LIBS}
    )

    target_link_libraries(hello PRIVATE ${Boost_LIBRARIES})

    add_executable(
        hello-vk
        main-vk-glfw.cpp
        main-glfw.h main-glfw.cpp
        visuals/indirect_draw-vk.cpp
    )

    add_executable(
        hello-gl
        main-gl-glfw.cpp
        main-glfw.h main-glfw.cpp
    )

    target_link_libraries(
        hello-gl PRIVATE
        OpenGL::GL
        glfw
    )

    target_link_libraries(
        hello-vk PRIVATE
        hello
        Vulkan::Vulkan
        glfw
    )


    add_executable(
        server
        server-main.cpp
        # TODO: put these in a separate library
        network/network_message.h network/network_message.cpp
    )

    target_compile_features(server PUBLIC cxx_std_20)

    target_link_libraries(server PRIVATE ${Boost_LIBRARIES} glm)
endif()

target_sources(
    hello-gl PRIVATE
    visuals/indirect_draw-gl.cpp
)

target_link_libraries(
    hello-gl PRIVATE
    hello
    vulkangl
)

target_compile_features(hello PUBLIC cxx_std_20)

#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
