#version 450
#pragma shader_stage(vertex)

layout (std140, binding = 0) uniform parameters {
    mat4 model_view_projection_matrix;
};

// TODO: read vertices from buffer
vec3 positions[] = vec3[](
    vec3(-1.0, -1.0,  1.0),
    vec3(1.0, -1.0,  1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(-1.0, -1.0, -1.0),
    vec3(1.0, -1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(1.0,  1.0, -1.0),

    vec3(-0.5, -0.5, -0.1),
    vec3(0.5, -0.5, -0.1),
    vec3(-0.5, 0.5, -0.1),
    vec3(0.5, 0.5, -0.1)
);

uint indices[] = uint[](
    8, 9, 10, 11, // plane
    0, 1, 2, 3, 7, 1, 5, 4, 7, 6, 2, 4, 0, 1 // cube
);

layout(location = 0) out vec2 uv;

void main() {
    uint index = indices[gl_VertexIndex];

    gl_Position = (
        model_view_projection_matrix *
        vec4(positions[index], 1.0)
    );
    uv = positions[index].xy + vec2(0.5);
}
