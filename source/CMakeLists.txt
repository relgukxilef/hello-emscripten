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
    state/file_cache.h state/file_cache.cpp
    state/model.h state/model.cpp
    utility/file.h utility/file.cpp
    utility/math.h
    audio/audio.h audio/audio.cpp
    utility/opus_resource.h utility/opus_resource.cpp
    utility/openal_resource.h utility/openal_resource.cpp
    utility/serialization.h utility/serialization.cpp
    utility/unique_span.h
    utility/trace.h utility/trace.cpp
    utility/vulkan_memory_allocator_resource.h
    utility/vulkan_memory_allocator_resource.cpp
)

target_shader(hello visuals/shaders/solid_vertex.glsl)
target_shader(hello visuals/shaders/solid_fragment.glsl)

target_link_libraries(
    hello PUBLIC
    Vulkan-Headers
    glm
    Boost::json
    Boost::static_string
    opus
    VulkanMemoryAllocator
)

if (EMSCRIPTEN)
    add_executable(
        hello-gl
        main-em.cpp
        network/websocket-em.cpp
        state/model-em.cpp
    )

    set_target_properties(hello-gl PROPERTIES SUFFIX ".js")
    set_target_properties(hello-gl PROPERTIES OUTPUT_NAME "index")

    add_custom_command(
        TARGET hello-gl POST_BUILD
        COMMAND 
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/html/index.html
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html
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
        -lopenal
        -sASYNCIFY
        -sEXPORTED_FUNCTIONS=['_main','_malloc']
        -sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU16,HEAPU32
    )

    target_link_options(
        hello PUBLIC
        "SHELL:--preload-file ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/visuals@visuals"
        "SHELL:--preload-file ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_files@test_files"
        -sINITIAL_MEMORY=1073741824
        -sALLOW_MEMORY_GROWTH
        -sMAXIMUM_MEMORY=4294967296
        #-fwasm-exceptions 
        -gsource-map=inline
    )
    target_compile_options(
        hello PUBLIC
        #-fwasm-exceptions 
        -gsource-map=inline
    )

else()
    target_sources(
        hello PUBLIC
        state/model-native.cpp
    )
    target_link_libraries(
       hello PUBLIC
       png_static
    )

    find_package(Vulkan 1.1)
    find_package(OpenGL)
    
    set(Boost_USE_STATIC_LIBS ON)
    find_package(OpenSSL REQUIRED)

    target_sources(
        hello PRIVATE
        network/websocket-boost.cpp
    )

    target_link_libraries(
        hello PRIVATE
        ${CMAKE_DL_LIBS}
        OpenSSL::SSL
        Boost::beast
        $<$<BOOL:${WIN32}>:wsock32>
        $<$<BOOL:${WIN32}>:ws2_32>
    )
    target_link_libraries(
        hello PUBLIC
        OpenAL::OpenAL
    )

    target_link_libraries(hello PRIVATE ${Boost_LIBRARIES})

    target_include_directories(hello PRIVATE ${Boost_INCLUDE_DIRS})

    if (Vulkan_FOUND)
        add_executable(
            hello-vk
            main-vk-glfw.cpp
            main-glfw.h main-glfw.cpp
            visuals/indirect_draw-vk.cpp
        )
    
        target_link_libraries(
            hello-vk PRIVATE
            hello
            Vulkan::Vulkan
            glfw
        )
    endif()

    if (OpenGL_FOUND)
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
    endif()


    add_executable(
        server
        server-main.cpp
        # TODO: put these in a separate library
        network/network_message.h network/network_message.cpp
    )

    target_compile_features(server PUBLIC cxx_std_20)

    target_link_libraries(
        server PRIVATE -static-libgcc -static-libstdc++ -static
    )

    target_link_libraries(
        server PRIVATE 
        ${Boost_LIBRARIES} 
        Boost::lexical_cast
        Boost::beast
        glm 
        $<$<BOOL:${UNIX}>:pthread>
        $<$<BOOL:${WIN32}>:wsock32>
        $<$<BOOL:${WIN32}>:ws2_32>
    )

    target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS})


    add_executable(
        test
        test-main.cpp
    )

    target_compile_features(test PUBLIC cxx_std_20)

    add_dependencies(test hello-gl hello-vk server)

    target_link_libraries(
        test PRIVATE 
        ${Boost_LIBRARIES}
        Boost::process
    )
endif()

add_custom_command(
    TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_files/AvatarSample_B.vrm
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_files/AvatarSample_B.vrm
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_files/white_modern_living_room.glb
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_files/white_modern_living_room.glb
)

if(OpenGL_FOUND OR EMSCRIPTEN)
    target_sources(
        hello-gl PRIVATE
        visuals/indirect_draw-gl.cpp
    )
    
    target_link_libraries(
        hello-gl PRIVATE
        hello
        vulkangl
    )
endif()

target_compile_features(hello PUBLIC cxx_std_20)

if (MSVC)
    add_definitions(/bigobj)
    add_compile_options(/fsanitize=address)
    add_link_options(/fsanitize=address)
endif ()
