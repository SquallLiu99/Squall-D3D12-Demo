#pragma once
#include <assert.h>
#include <d3d12.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <process.h>
using namespace std;

#include <wrl.h>
using namespace Microsoft::WRL;

// frame resource
#include "FrameResource.h"
#include "UploadBuffer.h"

// game time
#include "GameTimerManager.h"
#include "CameraManager.h"

// setup thread parameter
struct ThreadParameter
{
	int threadIndex;
};

class GraphicManager
{
public:
	GraphicManager(const GraphicManager&) = delete;
	GraphicManager(GraphicManager&&) = delete;
	GraphicManager& operator=(const GraphicManager&) = delete;
	GraphicManager& operator=(GraphicManager&&) = delete;

	static GraphicManager& Instance()
	{
		static GraphicManager instance;
		return instance;
	}

	GraphicManager() {}
	~GraphicManager() {}

	bool Initialize(ID3D12Device* _device, int _numOfThreads);
	void SetScreenSize(int _w, int _h);
	void InitRayTracingInterface();
	void InitSystemConstant();
	void Release();
	int GetThreadCount();
	void Update();
	void Render();
	void RenderThread();
	void WaitForRenderThread();
	void WaitForGPU();
	void ResetCreationList();
	void ExecuteCreationList();
	void ExecuteCommandList(ID3D12GraphicsCommandList* _cmdList);
	void CopyResourceWithBarrier(ID3D12GraphicsCommandList* _cmdList, ID3D12Resource* _src, ID3D12Resource* _dst, D3D12_RESOURCE_STATES _beforeCopy[2], D3D12_RESOURCE_STATES _afterCopy[2]);
	void ResolveColorBuffer(ID3D12GraphicsCommandList* _cmdList, ID3D12Resource* _src, ID3D12Resource* _dst, DXGI_FORMAT _format);

	ID3D12Device *GetDevice();
	ID3D12Device5* GetDxrDevice();
	ID3D12GraphicsCommandList5* GetDxrList();
	ID3D12QueryHeap *GetGpuTimeQuery();
	UINT GetRtvDesciptorSize();
	UINT GetDsvDesciptorSize();
	UINT GetCbvSrvUavDesciptorSize();
	FrameResource *GetFrameResource();
	UINT64 GetGpuFreq();
	void WaitBeginWorkerThread(int _index);
	void WakeAndWaitWorker();
	void SetWorkerThreadFinishEvent(int _index);
	void UploadSystemConstant(SystemConstant _sc);
	SystemConstant GetSystemConstantCPU();
	D3D12_GPU_VIRTUAL_ADDRESS GetSystemConstantGPU();
	void GetScreenSize(int& _w, int& _h);

private:
	HRESULT CreateGpuTimeQuery();
	HRESULT CreateGraphicCommand();
	HRESULT CreateGfxAlloc(ComPtr<ID3D12CommandAllocator>& _allocator);
	HRESULT CreateGfxList(ComPtr<ID3D12CommandAllocator>& _allocator, ComPtr<ID3D12GraphicsCommandList>& _list);
	HRESULT CreateGraphicFences();
	bool CreateGraphicThreads();
	void DrawCamera();

	ID3D12Device *mainDevice;
	ComPtr<ID3D12CommandQueue> mainGraphicQueue;
	ComPtr<ID3D12CommandAllocator> mainGfxAllocator[MAX_FRAME_COUNT];
	ComPtr<ID3D12GraphicsCommandList> mainGfxList;

	// allow multiple-thread gfx
	ComPtr<ID3D12CommandAllocator> workerGfxAllocator[MAX_WORKER_THREAD_COUNT][MAX_FRAME_COUNT];	
	ComPtr<ID3D12GraphicsCommandList> workerGfxList[MAX_WORKER_THREAD_COUNT];
	ComPtr<ID3D12Device5> rayTracingDevice;
	ComPtr<ID3D12GraphicsCommandList5> rayTracingCmd;

	// for gpu time measure
	ComPtr<ID3D12QueryHeap> gpuTimeQuery;
	UINT64 gpuFreq;

	// render thread
	int numOfLogicalCores;
	HANDLE beginRenderThread;
	HANDLE renderThreadHandle;
	HANDLE renderThreadFinish;
	HANDLE beginWorkerThread[MAX_WORKER_THREAD_COUNT];
	HANDLE workerThreadHandle[MAX_WORKER_THREAD_COUNT];
	HANDLE workerThreadFinish[MAX_WORKER_THREAD_COUNT];
	ThreadParameter threadParams[MAX_WORKER_THREAD_COUNT];

	// frame index and fences
	int currFrameIndex;
	ComPtr<ID3D12Fence> mainGraphicFence;
	UINT64 mainFence;
	HANDLE mainFenceEvent;				// fence event handle for sync
	UINT64 graphicFences[MAX_FRAME_COUNT];	// use for frame list

	// if init succeed
	bool initSucceed;

	// descriptor size
	UINT rtvDescriptorSize;
	UINT dsvDescriptorSize;
	UINT cbvSrvUavDescriptorSize;

	// system constant
	int screenWidth;
	int screenHeight;
	SystemConstant systemConstantCPU;
	FrameResource frameResource;
	DescriptorHeapData linearWrapSampler;
	DescriptorHeapData linearClampSampler;
	DescriptorHeapData anisotropicWrapSampler;

	// system constant
	unique_ptr<UploadBuffer<SystemConstant>> systemConstantGPU[MAX_FRAME_COUNT];
};