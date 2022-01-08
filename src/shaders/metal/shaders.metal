#include <simd/simd.h>

struct Vertex {
    float4 position [[position]];
    float4 color;
    float4 normals;
    float2 uv;
    float2 padding;
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
    const device CameraData* cameraData [[ buffer(1) ]],
    unsigned int vid [[ vertex_id ]]) {
    Vertex vertexOut = vertices[vid];
    vertexOut.position *= cameraData->projection;
    return vertexOut;
}

fragment float4 basic_fragment(Vertex in [[stage_in]]) {
    return in.color;
}
