#pragma once
#include <d3d12.h>
#include "stdafx.h"
#include <wrl.h>
#include <vector>
using namespace std;
using namespace Microsoft::WRL;

struct TextureInfo
{
	TextureInfo() { typeless = false; isCube = false; isUav = false; isMsaa = false; isBuffer = false; numElement = 0; numStride = 0; }
	TextureInfo(bool _typeless, bool _isCube, bool _isUav, bool _isMsaa, bool _isBuffer, UINT _numElement = 0, UINT _stride = 0)
	{
		typeless = _typeless;
		isCube = _isCube;
		isUav = _isUav;
		isMsaa = _isMsaa;
		isBuffer = _isBuffer;
		numElement = _numElement;
		numStride = _stride;
	}

	bool typeless;
	bool isCube;
	bool isUav;
	bool isMsaa;
	bool isBuffer;

	// use for structured buffer
	UINT numElement;
	UINT numStride;
};

class Texture
{
public:
	Texture(int _numRtvHeap, int _numDsvHeap);
	void SetInstanceID(size_t _id);
	size_t GetInstanceID();

	void Release();
	void SetResource(ID3D12Resource* _data);
	ID3D12Resource* GetResource();

	void SetFormat(DXGI_FORMAT _format);
	void SetInfo(TextureInfo _info);
	DXGI_FORMAT GetFormat();
	TextureInfo GetInfo();

	int InitRTV(ID3D12Resource* _rtv, DXGI_FORMAT _format, bool _msaa);
	int InitDSV(ID3D12Resource* _dsv, DXGI_FORMAT _format, bool _msaa);

	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCPU(int _index);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCPU(int _index);

	ID3D12Resource* GetRtvSrc(int _index);
	ID3D12Resource* GetDsvSrc(int _index);

protected:
	size_t instanceID;

	// use for texture manager (global heap)
	ID3D12Resource* texResource;
	DXGI_FORMAT texFormat;
	TextureInfo texInfo;

	// normally managed by manager, but allow indenpendent descriptor also
	ComPtr<ID3D12DescriptorHeap> rtvHandle;
	ComPtr<ID3D12DescriptorHeap> dsvHandle;

	vector<ID3D12Resource*> rtvSrc;
	vector<ID3D12Resource*> dsvSrc;

	int rtvCount;
	int dsvCount;
};