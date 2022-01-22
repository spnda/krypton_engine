#version 460

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in struct {
    vec4 color;
    vec2 uv;
} inp;

void main() {
    fragColor = inp.color * texture(sTexture, inp.uv.st);
}