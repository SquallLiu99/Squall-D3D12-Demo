#pragma sq_cbuffer SystemConstant
#pragma sq_srv _SqDirLight
#pragma sq_srv _ShadowMap

#include "SqInput.hlsl"
#pragma sq_vertex CollectShadowVS
#pragma sq_pixel CollectShadowPS

struct v2f
{
	float4 vertex : SV_POSITION;
    float4 screenPos : TEXCOOED0;
};

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

#pragma sq_srvStart
// shadow map, up to 4 cascades, 1st entry is camera's depth
Texture2D _ShadowMap[5] : register(t0);
#pragma sq_srvEnd

v2f CollectShadowVS(uint vid : SV_VertexID)
{
	v2f o = (v2f)0;

    // convert uv to ndc space
    float2 uv = gTexCoords[vid];
    o.vertex = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0, 1);
    o.screenPos = float4(o.vertex.xy, 0, 1);

	return o;
}

float4 CollectShadowPS(v2f i) : SV_Target
{
    float depth = _ShadowMap[0].Load(uint3(i.vertex.xy, 0)).r;
    [branch]
    if (depth == 0.0f)
    {
        // early jump out of fragment
        return 1.0f;
    }

    float3 wpos = DepthToWorldPos(depth, i.screenPos);

    return float4(wpos, 1.0f);
}