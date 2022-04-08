cmake_minimum_required(VERSION 3.5)

add_executable(
    hello-emscripten
    main.cpp
)

target_link_libraries(
    hello-emscripten PUBLIC 
    vulkangl
)

set_target_properties(hello-emscripten PROPERTIES SUFFIX ".html")