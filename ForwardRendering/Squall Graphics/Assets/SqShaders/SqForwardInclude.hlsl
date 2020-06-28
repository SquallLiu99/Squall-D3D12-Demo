#ifndef SQFORWARDINCLUDE
#define SQFORWARDINCLUDE
#include "SqLight.hlsl"

struct VertexInput
{
	float3 vertex : POSITION;
	float3 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv1 : TEXCOORD0;
	float2 uv2 : TEXCOORD1;
};

cbuffer SystemConstant : register(b0)
{
	float4x4 SQ_MATRIX_MVP;
};

cbuffer MaterialConstant : register(b1)
{
	float4 _MainTex_ST;
	float4 _Color;
	float4 _SpecColor;
	float4 _EmissionColor;
	float _CutOff;
	float _Smoothness;
	float _OcclusionStrength;
	int _DiffuseIndex;
	int _DiffuseSampler;
	int _SpecularIndex;
	int _SpecularSampler;
	int _OcclusionIndex;
	int _OcclusionSampler;
	int _EmissionIndex;
	int _EmissionSampler;
};

#pragma sq_srvStart
// need /enable_unbounded_descriptor_tables when compiling
Texture2D _TexTable[] : register(t0);
SamplerState _SamplerTable[] : register(s0);
StructuredBuffer<SqLight> _SqDirLight: register(t0, space1);
//StructuredBuffer<SqLight> _SqPointLight: register(t1, space1);
//StructuredBuffer<SqLight> _SqSpotLight: register(t2, space1);
#pragma sq_srvEnd

float4 GetSpecular(float2 uv)
{
#ifdef _SPEC_GLOSS_MAP
	return _TexTable[_SpecularIndex].Sample(_SamplerTable[_SpecularSampler], uv);
#else
	return float4(_SpecColor.rgb, _Smoothness);
#endif
}

float3 DiffuseAndSpecularLerp(float3 diffuse, float3 specular)
{
	return diffuse * (float3(1, 1, 1) - specular);
}

float GetOcclusion(float2 uv)
{
	float o = _TexTable[_OcclusionIndex].Sample(_SamplerTable[_OcclusionSampler], uv).g;
	return lerp(1, o, _OcclusionStrength);
}

float3 GetEmission(float2 uv)
{
#ifdef _EMISSION
	float3 emission = _TexTable[_EmissionIndex].Sample(_SamplerTable[_EmissionSampler], uv).rgb;
	return emission * _EmissionColor.rgb;
#else
	return 0;
#endif
}

float3 AccumulateDirLight()
{
	uint numLight = 0;
	uint stride = 0;
	_SqDirLight.GetDimensions(numLight, stride);

	float3 col = 0;
	for (uint i = 0; i < 1; i++)
	{
		col += _SqDirLight[i].color * _SqDirLight[i].intensity;
	}

	return col;
}

float3 LightBRDF(float3 diffColor)
{
	return diffColor * AccumulateDirLight();
}

#endif