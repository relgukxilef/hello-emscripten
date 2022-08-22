#version 450
#pragma shader_stage(vertex)

vec2 positions[] = vec2[](
    vec2(-0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, -0.5),
    vec2(0.5, 0.5)
);

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.1, 0.0).xzyy;
    uv = positions[gl_VertexIndex] + vec2(0.5);
}
