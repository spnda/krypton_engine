#version 460

layout(location = 0) in vec4 pos;

layout(location = 0) out struct {
    vec4 color;
} ou;

void main() {
    gl_Position = pos;
    ou.color = vec4(0.0, 0.5, 0.5, 1.0);
}
