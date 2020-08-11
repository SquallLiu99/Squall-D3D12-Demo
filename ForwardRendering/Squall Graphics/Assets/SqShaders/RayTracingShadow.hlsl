// need assign relative path for dxc compiler with forward slash
#include "Assets/SqShaders/SqInput.hlsl"
#pragma sq_rayrootsig RTShadowRootSig
#pragma sq_raygen RTShadowRayGen
#pragma sq_closesthit RTShadowClosestHit
#pragma sq_anyhit RTShadowAnyHit
#pragma sq_miss RTShadowMiss
#pragma sq_hitgroup RTShadowGroup
#pragma sq_rtshaderconfig RTShadowConfig
#pragma sq_rtpipelineconfig RTShadowPipelineConfig

GlobalRootSignature RTShadowRootSig =
{
    "DescriptorTable( UAV( u0 , numDescriptors = 1) ),"     // raytracing output
    "DescriptorTable( SRV( t0 , numDescriptors = 1) ),"     // depth map
    "CBV( b1 ),"                        // system constant
    "SRV( t0, space = 2),"              // acceleration strutures
    "SRV( t0, space = 1 )"              // sqlight
};

TriangleHitGroup RTShadowGroup =
{
    "RTShadowAnyHit",      // AnyHit
    "RTShadowClosestHit"   // ClosestHit
};

RaytracingShaderConfig RTShadowConfig =
{
    16, // max payload size - 4float
    8   // max attribute size - 2float
};

RaytracingPipelineConfig RTShadowPipelineConfig =
{
    1 // max trace recursion depth
};

struct RayPayload
{
    float atten;
    float3 padding;
};

RaytracingAccelerationStructure _SceneAS : register(t0, space2);
RWTexture2D<float4> _OutputShadow : register(u0);
Texture2D _DepthMap : register(t0);

[shader("raygeneration")]
void RTShadowRayGen()
{
    // center in the middle of the pixel, it's half-offset rule of D3D
    float2 xy = DispatchRaysIndex().xy + 0.5f; 

    // to ndc space
    float2 screenUV = (xy / DispatchRaysDimensions().xy);
    screenUV.y = 1 - screenUV.y;
    screenUV = screenUV * 2.0f - 1.0f;

    // depth
    float depth = _DepthMap.Load(uint3(DispatchRaysIndex().xy, 0)).r;

    [branch]
    if (depth == 0.0f)
    {
        // early out
        _OutputShadow[DispatchRaysIndex().xy] = 1;
        return;
    }

    // to world pos
    float3 wpos = DepthToWorldPos(depth, float4(screenUV, 0, 1));

    // setup ray, trace for main dir light
    SqLight mainLight = _SqDirLight[0];

    RayDesc ray;
    ray.Origin = wpos;
    ray.Direction = -mainLight.world.xyz;   // shoot a ray to light
    ray.TMin = mainLight.world.w;           // use bias as t min
    ray.TMax = mainLight.cascadeDist[0];

    // the data payload between ray tracing
    RayPayload payload = { float4(1, 1, 1, 1) };
    TraceRay(_SceneAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, ray, payload);

    // output shadow
    _OutputShadow[DispatchRaysIndex().xy] = payload.atten;
}

[shader("closesthit")]
void RTShadowClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    // hit, set shadow atten
    payload.atten = 0.0f;
}

[shader("anyhit")]
void RTShadowAnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    // hit transparent
    payload.atten = 0.0f;

    // finish any hit
    AcceptHitAndEndSearch();
}

[shader("miss")]
void RTShadowMiss(inout RayPayload payload)
{
    // hit nothing at all, no shadow atten
    payload.atten = 1.0f;
}