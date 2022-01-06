struct Vertex {
    float4 position [[position]];
    float4 color;
    float4 normals;
    float2 uv;
    float2 padding;
};

// Index based vertex shader
vertex Vertex basic_vertex(
    const device Vertex* vertices [[ buffer(0) ]],
    unsigned int vid [[ vertex_id ]]) {
    return vertices[vid];
}

fragment float4 basic_fragment(Vertex in [[stage_in]]) {
    return in.color;
}
