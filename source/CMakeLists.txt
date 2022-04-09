cmake_minimum_required(VERSION 3.5)

add_executable(
    hello-emscripten
    main.cpp
    utility/resource.h
    window/window.h
    window/window-$<IF:$<BOOL:EMSCRIPTEN>,ems,win32>.cpp
)

if (EMSCRIPTEN)
    target_link_libraries(
        hello-emscripten PUBLIC 
        vulkangl
    )
    
    set_target_properties(hello-emscripten PROPERTIES SUFFIX ".html")
endif()