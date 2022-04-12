#include <metal_stdlib>

struct Vertex {
    float4 position [[position]];
    float4 color;
    float4 normals;
    float2 uv;
    float2 padding;
};

struct GBufferFragmentOut {
    half4 color [[color(0)]];
    half4 normals [[color(1)]];
    half4 albedo [[color(2)]];
};

struct Material {
    metal::texture2d<float> albedo [[id(0)]];
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
vertex Vertex basic_vertex(
        const device Vertex* vertices [[ buffer(0) ]],
        const device InstanceData* instance [[ buffer(1) ]],
        const device CameraData& cameraData [[ buffer(2) ]],
        unsigned int vid [[ vertex_id ]],
        unsigned int iid [[ instance_id ]]) {
    Vertex vertexOut = vertices[vid];
    vertexOut.position = instance->instanceTransform * vertexOut.position;
    vertexOut.position = cameraData.projection * cameraData.view * vertexOut.position;
    return vertexOut;
}

// ,
//                                           constant Material& material [[ buffer(0) ]])
fragment GBufferFragmentOut basic_fragment(Vertex in [[stage_in]]) {
    GBufferFragmentOut out;
    out.color = half4(in.color.r, in.color.g, in.color.b, 1.0);
    out.normals = half4(in.normals);
    out.albedo = half4(in.color);
    return out;
}
