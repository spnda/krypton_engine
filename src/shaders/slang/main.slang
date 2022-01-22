// main.slang

struct CameraData {
    float4x4 projectionInverse;
    float4x4 viewInverse;
    float near;
    float far;
};

struct RayPayload {
    float4 color;
};

// slang re-orders these arguments somehow. In the future
// we should use reflection to scan for the parameters.
// For now, I'll write these in the real order, from 0 to 2. 
uniform CameraData camera;
uniform RaytracingAccelerationStructure sceneBVH;
uniform RWTexture2D storageImage;

float3 getRayDirection(uint2 id, in float offset) {
    float2 pixel = id.xy + offset;
    float2 inUV = pixel / id.xy;
    float2 d = inUV * 2.0 - 1.0;
    float4 target = mul(camera.projectionInverse, float4(d.x, d.y, 1.0, 1.0));
    return mul(camera.viewInverse, float4(normalize(target.xyz), 0.0)).xyz;
}

[shader("raygeneration")]
void raygen() {
    uint2 threadIdx = DispatchRaysIndex().xy;
    uint2 size = DispatchRaysDimensions().xy;
    // float3 direction = getRayDirection(threadIdx, 0.0);
    float3 dimension = 2 * float3(threadIdx / float2(size), 0.61) - 1;

    RayDesc ray;
    ray.Origin = dimension;
    ray.Direction = float3(0, 0, -1);
    ray.TMin = camera.near;
    ray.TMax = camera.far;

    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(sceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

    storageImage[threadIdx.xy] = payload.color;
}

[shader("miss")]
void miss(inout RayPayload payload) {
    payload.color = float4(0, 0.5, 0, 1);
}

[shader("closesthit")]
void closesthit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes) {
    float3 hitLocation = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    payload.color = float4(1.0, 0.5, 0.5, 1.0);
}
