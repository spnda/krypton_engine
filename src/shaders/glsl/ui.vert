#version 460

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

// We specifically specify 'buffer' here to force SPIRV-Cross to use a device-scope pointer.
layout(set = 1, binding = 0) buffer Uniforms {
    vec2 scale;
    vec2 translate;
} uniforms;

layout(location = 0) out struct {
    vec4 color;
    vec2 uv;
} outp;

void main() {
    outp.color = inColor;
    outp.uv = inUV;
    gl_Position = vec4(inPos * uniforms.scale + uniforms.translate, 0, 1) * vec4(1.0f, -1.0f, 1.0f, 1.0f);
}
