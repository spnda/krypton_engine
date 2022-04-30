#include <metal_stdlib>

// Basic sampler
constexpr metal::sampler textureSampler(metal::mip_filter::linear,
                                        metal::mag_filter::linear,
                                        metal::min_filter::linear,
                                        metal::s_address::repeat,
                                        metal::t_address::repeat);

struct Vertex {
    float4 position [[position]];
    float4 color;
    float4 normals;
    float2 uv;
    float2 padding;
};

struct FSTVertex {
    float4 position [[position]];
    float2 texCoord;
};

struct GBufferFragmentOut {
    half4 normals [[color(0)]];
    half4 albedo [[color(1)]];
};

struct FinalFragmentOut {
    half4 color [[color(0)]];
};

struct Material {
    metal::texture2d<float, metal::access::sample> albedo [[id(0)]];
};

struct InstanceData {
    metal::float4x4 instanceTransform;
};

struct CameraData {
    metal::float4x4 projection;
    metal::float4x4 view;
    float near;
    float far;
};

// Index based vertex shader
vertex Vertex gbuffer_vertex(
    const device Vertex* vertices [[ buffer(0) ]],
    const device InstanceData* instance [[ buffer(1) ]],
    const device CameraData& cameraData [[ buffer(2) ]],
    uint vid [[ vertex_id ]],
    uint iid [[ instance_id ]]) {
    Vertex vertexOut = vertices[vid];
    vertexOut.position = instance->instanceTransform * vertexOut.position;
    vertexOut.position = cameraData.projection * cameraData.view * vertexOut.position;
    return vertexOut;
}

fragment GBufferFragmentOut gbuffer_fragment(Vertex in [[stage_in]],
                                           constant Material* material [[ buffer(0) ]]) {
    GBufferFragmentOut out;
    out.normals = half4(in.normals);
    out.albedo = half4(material->albedo.sample(textureSampler, in.uv));
    return out;
}

// Vertex shader for rendering a full screen quad.
vertex FSTVertex fsq_vertex(uint vid [[vertex_id]]) {
    FSTVertex out = {};
    out.texCoord = float2((vid << 1) & 2, vid & 2);
    out.position = float4(out.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);
    return out;
}

fragment FinalFragmentOut final_fragment(FSTVertex in [[stage_in]],
                                         metal::texture2d<float, metal::access::sample> normals [[texture(0)]],
                                         metal::texture2d<float, metal::access::sample> albedo [[texture(1)]]) {
    FinalFragmentOut out = {};
    out.color = half4(albedo.sample(textureSampler, in.texCoord));
    return out;
}
