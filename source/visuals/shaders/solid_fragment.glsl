#version 450
#pragma shader_stage(fragment)

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 uv;

vec2 filtered_step(vec2 x, vec2 width) {
    return clamp(x / width + vec2(0.5), vec2(0), vec2(1));
}

void main() {
    color = vec4(0.11,0.839,0.808, 1.0);

    vec2 p = uv * 4 + 0.25;
    p = filtered_step(min(fract(p), 1 - fract(p)) - 0.25, fwidth(p));
    
    color *= mix(p.y, 1 - p.y, p.x);
}
