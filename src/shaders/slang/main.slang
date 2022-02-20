// main.slang

struct CameraData {
    float4x4 projectionInverse;
    float4x4 viewInverse;
    float near;
    float far;
};

struct GeometryDescription {
    uint64_t vertexBufferAddress = 0;
    uint64_t indexBufferAddress = 0;
    uint32_t materialIndex = 0;
};

struct Material {
    float4 baseColor;

    float metallicFactor;
    float roughnessFactor;

    int baseTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
    int pbrTextureIndex;
};

struct RayPayload {
    float4 color;
};

// We can use vk::binding or vk_binding to specify the layout
// bindings manually. First comes the binding, then the descriptor set.
[[vk::binding(0, 0)]] uniform CameraData camera;
[[vk::binding(1, 0)]] uniform StructuredBuffer<GeometryDescription> geometry;

// I'd prefer if our material buffer was not a free list on the GPU as well,
// but it makes the copying process so much simpler.
[[vk::binding(2, 0)]] uniform StructuredBuffer<Material> materials;

[[vk::binding(3, 0)]] uniform RaytracingAccelerationStructure sceneBVH;
[[vk::binding(4, 0)]] uniform RWTexture2D storageImage;

float3 getRayDirection(uint2 id, uint2 size, in float offset) {
    float2 inUV = (id.xy + offset) / size.xy;
    float2 d = inUV * 2.0 - 1.0;
    float4 target = mul(camera.projectionInverse, float4(d.x, d.y, 1.0, 1.0));
    return mul(camera.viewInverse, float4(normalize(target.xyz), 0.0)).xyz;
}

[shader("raygeneration")]
void raygen() {
    uint2 threadIdx = DispatchRaysIndex().xy;
    uint2 size = DispatchRaysDimensions().xy;

    float3 direction = getRayDirection(threadIdx, size, 0.0);

    RayDesc ray;
    ray.Origin = mul(camera.viewInverse, float4(0, 0, 0, 1)).xyz;
    ray.Direction = direction;
    ray.TMin = camera.near;
    ray.TMax = camera.far;

    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(sceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

    storageImage[threadIdx.xy] = payload.color;
}

[shader("miss")]
void miss(inout RayPayload payload) {
    payload.color = float4(0.2, 0.2, 0.2, 1);
}

[shader("closesthit")]
void closesthit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes intersectionAttributes) {
    float3 hitLocation = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    GeometryDescription description = geometry[InstanceIndex() + GeometryIndex()];

    float2 attributes = intersectionAttributes.barycentrics;
    float3 baryCoords = float3(1.0 - attributes.x - attributes.y, attributes.x, attributes.y);

    float4 color = materials[description.materialIndex].baseColor;
    payload.color = color;
}
