cmake_minimum_required(VERSION 3.5)

add_executable(
    hello-emscripten
    main.cpp
    utility/resource.h
    window/window.h
    window/window-$<IF:$<BOOL:EMSCRIPTEN>,ems,win32>.cpp
    utility/vulkan_resource.h
    utility/vulkan_resource.cpp
)

if (EMSCRIPTEN)
    target_link_libraries(
        hello-emscripten PRIVATE 
        vulkangl
    )
    
    set_target_properties(hello-emscripten PROPERTIES SUFFIX ".html")

else()
    target_link_libraries(
        hello-emscripten PRIVATE 
        vulkan-1
    )
endif()