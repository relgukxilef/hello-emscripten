
function(target_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(input_path ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(output_path ${CMAKE_CURRENT_BINARY_DIR}/${SHADER})

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(output_directory ${output_path} DIRECTORY)
    file(MAKE_DIRECTORY ${output_directory})

    add_custom_command(
        OUTPUT ${output_path}.spv
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND 
        ${GLSLC} --target-env=vulkan1.0 -O -o ${SHADER}.spv ${input_path}
        DEPENDS ${input_path}
        VERBATIM
    )
    target_sources(
        ${TARGET} PRIVATE ${output_path}.spv ${input_path}
    )
endfunction(target_shader)
