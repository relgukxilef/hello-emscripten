#version 450
#pragma shader_stage(vertex)

layout (std140, binding = 0) uniform parameters {
    mat4 model_view_projection_matrix;
};

layout (location = 0) in vec3 position;

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = (
        model_view_projection_matrix * vec4(position, 1.0)
    );
    uv = position.xy + vec2(0.5);
}
