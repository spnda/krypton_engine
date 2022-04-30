#version 460

layout (location = 0) out vec4 fragColor;

layout(location = 0) in struct {
    vec4 color;
    vec2 uv;
} inp;

void main() {
	fragColor = vec4(inp.color);
}
