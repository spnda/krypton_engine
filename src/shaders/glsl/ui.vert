#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_gpu_shader_int64 : require

struct ImDrawVert {
    vec2 pos;
    vec2 uv;
    uint col;
};

layout(buffer_reference, scalar) buffer Vertices { ImDrawVert v[]; };

layout(set = 0, binding = 0) uniform Uniforms {
    vec2 scale;
    vec2 translate;
} uniforms;

layout(location = 0) out struct {
    vec4 color;
    vec2 uv;
} outp;

layout(push_constant) uniform constants {
	uint64_t vertexBufferAddress;
} PushConstants;

void main() {
    Vertices vertices = Vertices(PushConstants.vertexBufferAddress);
    ImDrawVert vert = vertices.v[gl_VertexIndex];
    outp.color = unpackUnorm4x8(vert.col);
    outp.uv = vert.uv;
    gl_Position = vec4(vert.pos * uniforms.scale + uniforms.translate, 0, 1) * vec4(1.0f, -1.0f, 1.0f, 1.0f);
}
