#define ForwardPlusTileRS "RootFlags(0)," \
"DescriptorTable(UAV(u0, numDescriptors=1))," \
"CBV(b1)," \
"SRV(t1, space=1)," \
"DescriptorTable( SRV( t0 , numDescriptors = unbounded) )," \
"DescriptorTable( Sampler( s0 , numDescriptors = unbounded) )"

#pragma sq_compute ForwardPlusTileCS
#pragma sq_rootsig ForwardPlusTileRS
#include "SqInput.hlsl"

// result
RWByteAddressBuffer _TileResult : register(u0);

groupshared uint minDepthU;
groupshared uint maxDepthU;
groupshared uint tilePointLightCount;
groupshared uint tilePointLightArray[1024];

// similiar to BoundingFrustum::CreateFromMatrix()
void CalcFrustumPlanes(uint tileX, uint tileY, float2 tileBias, float minZ, float maxZ, out Plane plane[6])
{
	float x = -1.0f + tileBias.x * tileX;
	float y = -1.0f + tileBias.y * tileY;
	float rx = x + tileBias.x;
	float ry = y + tileBias.y;

	// frustum corner in NDC space (at far plane) - LB RB LT RT
	float4 corners[6];
	corners[0] = float4(x, y, maxZ, 1.0f);
	corners[1] = float4(rx, y, maxZ, 1.0f);
	corners[2] = float4(x, ry, maxZ, 1.0f);
	corners[3] = float4(rx, ry, maxZ, 1.0f);

	// near far
	corners[4] = float4(0, 0, minZ, 1.0f);
	corners[5] = float4(0, 0, maxZ, 1.0f);

	// convert corners to world position
	[unroll]
	for (uint i = 0; i < 6; i++)
	{
		corners[i].xyz = DepthToWorldPos(corners[i].z, corners[i]);
	}

	// dir from camera to corners
	[unroll]
	for (i = 0; i < 4; i++)
	{
		corners[i].xyz = normalize(corners[i].xyz - _CameraPos.xyz);
	}

	// plane order: Left, Right, Bottom, Top, Near, Far
	plane[0].normal = cross(corners[2].xyz, corners[0].xyz);
	plane[0].distance = 0;

	plane[1].normal = -cross(corners[3].xyz, corners[1].xyz);
	plane[1].distance = 0;

	plane[2].normal = cross(corners[0].xyz, corners[1].xyz);
	plane[2].distance = 0;

	plane[3].normal = -cross(corners[2].xyz, corners[3].xyz);
	plane[3].distance = 0;

	plane[4].normal = _CameraDir.xyz;
	plane[4].distance = abs(_CameraPos.z - corners[4].z);

	plane[5].normal = -_CameraDir.xyz;
	plane[5].distance = abs(_CameraPos.z - corners[5].z);
}

// use 32x32 threads per group
[RootSignature(ForwardPlusTileRS)]
[numthreads(32, 32, 1)]
void ForwardPlusTileCS(uint3 _globalID : SV_DispatchThreadID, uint3 _groupID : SV_GroupID, uint _threadIdx : SV_GroupIndex)
{
	float depth = -1;

	if (_globalID.x >= _ScreenSize.x || _globalID.y >= _ScreenSize.y)
	{
		// out of range
		// do nothing here, we need to check depth within a tile
	}
	else
	{
		// fetch depth
		depth = _TexTable[_DepthIndex][_globalID.xy].r;
	}

	// init thread variable and sync
	if (_threadIdx == 0)
	{
		minDepthU = UINT_MAX;
		maxDepthU = 0;
		tilePointLightCount = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	// find min/max depth
	if (depth != -1.0)
	{
		uint depthU = asuint(depth);

		InterlockedMin(minDepthU, depthU);
		InterlockedMax(maxDepthU, depthU);
	}
	GroupMemoryBarrierWithGroupSync();

	// tile data
	uint tileCountX = ceil(_ScreenSize.x / 32);
	uint tileCountY = ceil(_ScreenSize.y / 32);
	uint tileIndex = _groupID.x + _groupID.y * tileCountX;
	float2 tileBias = float2(2.0f / tileCountX, 2.0f / tileCountY);

	// if thread not out-of-range
	if (_threadIdx < _NumPointLight)
	{
		float minDepthF = asfloat(minDepthU);
		float maxDepthF = asfloat(maxDepthU);
		if (maxDepthF - minDepthF < FLOAT_EPSILON)
		{
			// prevent zero range depth
			maxDepthF = minDepthF + FLOAT_EPSILON;
		}

		Plane plane[6];
		CalcFrustumPlanes(_groupID.x, _groupID.y, tileBias, maxDepthF, minDepthF, plane);

		// light overlap test
		SqLight light = _SqPointLight[_threadIdx];

		bool overlapping = true;
		for (int n = 0; n < 6; n++)
		{
			float d = dot(light.world.xyz, plane[n].normal) + plane[n].distance;
			if (d + light.range < 0)
			{
				overlapping = false;
			}
		}

		if (overlapping)
		{
			uint idx = 0;
			InterlockedAdd(tilePointLightCount, 1, idx);
			tilePointLightArray[idx] = _threadIdx;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	// output by 1st thread
	if (_threadIdx == 0)
	{
		uint tileOffset = tileIndex * (_NumPointLight * 4 + 4);
		_TileResult.Store(tileOffset, tilePointLightCount);

		uint offset = tileOffset + 4;
		for (uint i = 0; i < tilePointLightCount; i++)
		{
			_TileResult.Store(offset, tilePointLightArray[i]);
			offset += 4;
		}
	}
}