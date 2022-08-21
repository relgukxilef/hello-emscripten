#version 450
#pragma shader_stage(fragment)

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 uv;

float filtered_step(float x) {
    float width = fwidth(x);
    return clamp(x / width + 0.5, 0, 1);
}

void main() {
    color = vec4(0.11,0.839,0.808, 1.0);
    color *=
        vec4(filtered_step((fract(uv.x * 4) - 0.5) * (fract(uv.y * 4) - 0.5)));
}
