#version 460

layout(set = 2, binding = 0) uniform texture2D sTexture;
layout(set = 2, binding = 1) uniform sampler textureSampler;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in struct {
    vec4 color;
    vec2 uv;
} inp;

void main() {
    // imgui input colours are all encoded in sRGB.
    vec4 color = vec4(pow(inp.color.rgb / 255.0f, vec3(2.2)), inp.color.a / 255.0f);
    fragColor = color * texture(sampler2D(sTexture, textureSampler), inp.uv.st);
}
