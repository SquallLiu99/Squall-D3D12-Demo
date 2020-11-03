#pragma once
#include <d3d12.h>
#include "../Material.h"
#include "../TextureManager.h"
#include "../stdafx.h"
#include "../Camera.h"
#include "../UploadBuffer.h"

struct AmbientConstant
{
	float diffuseDist;
	float diffuseFadeDist;
	float diffuseStrength;
	float occlusionDist;
	float occlusionFadeDist;
	float occlusionStrength;
	float noiseTiling;
	float blurDepthThres;
	float blurNormalThres;
	int sampleCount;
	int ambientNoiseIndex;
	int blurRadius;
};

class RayAmbient
{
public:
	void Init(ID3D12Resource* _ambientRT, ID3D12Resource* _noiseTex);
	void Release();
	void Trace(Camera* _targetCam, D3D12_GPU_VIRTUAL_ADDRESS _dirLightGPU);
	void UpdataAmbientData(AmbientConstant _ac);

	int GetAmbientSrv();
	int GetAmbientNoiseSrv();
	Material* GetMaterial();

private:
	static const int maxSampleCount = 64;

	void CreateResource();
	D3D12_GPU_DESCRIPTOR_HANDLE GetAmbientUav();
	D3D12_GPU_DESCRIPTOR_HANDLE GetAmbientSrvHandle();

	ID3D12Resource* ambientSrc;
	Material rtAmbientMat;

	DescriptorHeapData ambientHeapData;
	DescriptorHeapData noiseHeapData;
	AmbientConstant ambientConst;
	unique_ptr<UploadBuffer<AmbientConstant>> ambientConstantGPU;
};