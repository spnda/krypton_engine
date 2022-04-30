#version 460

layout(set = 2, binding = 0) uniform texture2D sTexture;
layout(set = 2, binding = 1) uniform sampler textureSampler;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in struct {
    vec4 color;
    vec2 uv;
} inp;

void main() {
    fragColor = inp.color * texture(sampler2D(sTexture, textureSampler), inp.uv.st);
}
