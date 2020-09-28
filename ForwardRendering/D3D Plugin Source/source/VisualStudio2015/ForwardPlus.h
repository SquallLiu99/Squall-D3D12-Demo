#pragma once
#include "DefaultBuffer.h"
#include "Material.h"

class ForwardPlus
{
public:
	void Init(int _maxPointLight);
	void Release();

	D3D12_GPU_DESCRIPTOR_HANDLE GetLightCullingUav();
	D3D12_GPU_DESCRIPTOR_HANDLE GetLightCullingSrv();
	D3D12_GPU_DESCRIPTOR_HANDLE GetLightCullingTransUav();
	D3D12_GPU_DESCRIPTOR_HANDLE GetLightCullingTransSrv();
	void GetTileCount(int& _x, int& _y);
	ID3D12Resource* GetPointLightTileSrc();
	ID3D12Resource* GetPointLightTileTransSrc();

	void TileLightCulling(D3D12_GPU_VIRTUAL_ADDRESS _pointLightGPU);

private:
	// forward+ component
	unique_ptr<DefaultBuffer> pointLightTiles;
	unique_ptr<DefaultBuffer> pointLightTilesTrans;
	int tileSize = 32;
	int tileCountX;
	int tileCountY;
	int pointLightTileUav;
	int pointLightTileSrv;
	int pointLightTransTileUav;
	int pointLightTransTileSrv;
	Material forwardPlusTileMat;
};