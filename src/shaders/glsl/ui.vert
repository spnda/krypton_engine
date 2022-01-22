#version 460

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 translate;
} constants;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out struct {
    vec4 color;
    vec2 uv;
} outp;

void main() {
    outp.color = inColor;
    outp.uv = inUV;
    gl_Position = vec4(inPos * constants.scale + constants.translate, 0, 1);
}
