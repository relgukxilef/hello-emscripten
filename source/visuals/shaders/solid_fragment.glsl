#version 450
#pragma shader_stage(fragment)

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 fragment_texture_coordinate;
layout(location = 1) in vec3 fragment_normal;

vec2 filtered_step(vec2 x, vec2 width) {
    return clamp(x / width + vec2(0.5), vec2(0), vec2(1));
}

void main() {
    color = vec4(fragment_texture_coordinate, 1.0, 1.0);
    color *= dot(fragment_normal, vec3(1, 0, 0)) * 0.5 + 0.5;
}
