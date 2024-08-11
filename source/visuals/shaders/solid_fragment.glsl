#version 450
#pragma shader_stage(fragment)

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 fragment_texture_coordinate;
layout(location = 1) in vec3 fragment_normal;
layout(location = 2) in vec3 debug_color;

layout(binding = 1) uniform sampler2D image;

float filtered_step(float x, float width) {
    return clamp(x / width + 0.5, 0.0, 1.0);
}

void main() {
    color = texture(image, fragment_texture_coordinate);
    color.a = filtered_step(color.a - 0.5, fwidth(color.a));
    //color.rgb *= dot(fragment_normal, vec3(1, 0, 0)) * 0.5 + 0.5;
    color.rgb = debug_color;
}
