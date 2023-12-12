#version 450
#pragma shader_stage(vertex)

layout (std140, binding = 0) uniform parameters {
    mat4 model_view_projection_matrix;
    mat4 model_matrix;
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;

layout(location = 0) out vec2 fragment_texture_coordinate;
layout(location = 1) out vec3 fragment_normal;

void main() {
    gl_Position = (
        model_view_projection_matrix * vec4(position, 1.0)
    );
    fragment_texture_coordinate = texture_coordinate;
    fragment_normal = mat3(model_matrix) * normal;
}
