cmake_minimum_required(VERSION 3.13)

add_executable(
    hello-emscripten
    hello.h hello.cpp
    utility/out_ptr.h
    utility/resource.h
    utility/vulkan_resource.h utility/vulkan_resource.cpp
    visuals/visuals.h visuals/visuals.cpp
    visuals/view.h visuals/view.cpp
)

target_link_libraries(
    hello-emscripten PRIVATE
    Vulkan-Headers
)

if (EMSCRIPTEN)
    target_sources(
        hello-emscripten PUBLIC
        main-em.cpp
    )

    target_link_libraries(
        hello-emscripten PRIVATE 
        vulkangl
    )

    set_target_properties(hello-emscripten PROPERTIES SUFFIX ".html")
    set_target_properties(hello-emscripten PROPERTIES OUTPUT_NAME "index")
    target_compile_options(hello-emscripten PRIVATE -fsanitize=address)
    target_link_options(
        hello-emscripten PRIVATE -fsanitize=address -sNO_EXIT_RUNTIME
    )

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

target_compile_features(hello-emscripten PUBLIC cxx_std_20)

target_compile_options(hello-emscripten PRIVATE /Od /Wall)
