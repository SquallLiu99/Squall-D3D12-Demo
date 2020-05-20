#include "GraphicManager.h"
#include "stdafx.h"
#include "ForwardRenderingPath.h"
#include "d3dx12.h"
#include "RendererManager.h"

bool GraphicManager::Initialize(ID3D12Device* _device, int _numOfThreads)
{
	initSucceed = false;
	numOfLogicalCores = 0;

	mainDevice = _device;
#if defined(GRAPHICTIME)
	LogIfFailedWithoutHR(mainDevice->SetStablePowerState(true));
#endif

	// require logical cores number
	numOfLogicalCores = (int)std::thread::hardware_concurrency();
	numOfLogicalCores = (_numOfThreads > numOfLogicalCores) ? numOfLogicalCores : _numOfThreads;

	// reset frame index
	currFrameIndex = 0;

	// get descriptor size
	rtvDescriptorSize = mainDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = mainDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbvSrvUavDescriptorSize = mainDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	initSucceed = SUCCEEDED(CreateGpuTimeQuery())
		&& SUCCEEDED(CreateGraphicCommand())
		&& SUCCEEDED(CreateGraphicFences())
		&& CreateGraphicThreads();
    
	return initSucceed;
}

void GraphicManager::Release()
{
	WaitForGPU();

	mainGraphicFence.Reset();
	CloseHandle(mainFenceEvent);
	CloseHandle(beginRenderThread);
	CloseHandle(renderThreadHandle);
	CloseHandle(renderThreadFinish);

	for (int i = 0; i < MAX_WORKER_THREAD_COUNT; i++)
	{
		CloseHandle(beginWorkerThread);
		CloseHandle(workerThreadHandle[i]);
		CloseHandle(workerThreadFinish[i]);
	}

	for (int i = 0; i < MAX_FRAME_COUNT; i++)
	{
		mainGraphicAllocator[i].Reset();
		mainGraphicList[i].Reset();
	}


	mainGraphicQueue.Reset();
	gpuTimeQuery.Reset();
	gpuTimeResult.Reset();

	mainDevice = nullptr;
}

int GraphicManager::GetThreadCount()
{
	return numOfLogicalCores;
}

void GraphicManager::Update()
{
#if defined(GRAPHICTIME)
	TIMER_INIT
	TIMER_START
#endif
	
	const UINT64 lastCompletedFence = mainGraphicFence->GetCompletedValue();
	currFrameIndex = (currFrameIndex + 1) % MAX_FRAME_COUNT;

	// Make sure GPU isn't busy.
	// If it is, wait for it to complete.
	if (graphicFences[currFrameIndex] > lastCompletedFence)
	{
		HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		mainGraphicFence->SetEventOnCompletion(graphicFences[currFrameIndex], eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
	}

#if defined(GRAPHICTIME)
	TIMER_STOP
	GameTimerManager::Instance().gameTime.updateTime = elapsedTime;
#endif
}

void GraphicManager::Render()
{
#if defined(GRAPHICTIME)
	TIMER_INIT
	TIMER_START
	GameTimerManager::Instance().gameTime.gpuTime = 0.0f;
	GameTimerManager::Instance().gameTime.batchCount = 0;
#endif

	// wake up render thread
	ResetEvent(renderThreadFinish);
	SetEvent(beginRenderThread);
	WaitForRenderThread();

#if defined(GRAPHICTIME)
	TIMER_STOP
	GameTimerManager::Instance().gameTime.renderTime = elapsedTime;
#endif
}

HRESULT GraphicManager::CreateGpuTimeQuery()
{
	HRESULT hr = S_OK;

#if defined(GRAPHICTIME)
	D3D12_QUERY_HEAP_DESC queryDesc;
	ZeroMemory(&queryDesc, sizeof(queryDesc));
	queryDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	queryDesc.Count = 2;

	LogIfFailed(mainDevice->CreateQueryHeap(&queryDesc, IID_PPV_ARGS(&gpuTimeQuery)), hr);
	
	LogIfFailed(mainDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK)
		, D3D12_HEAP_FLAG_NONE
		, &CD3DX12_RESOURCE_DESC::Buffer(2 * sizeof(uint64_t))
		, D3D12_RESOURCE_STATE_COPY_DEST
		, nullptr
		, IID_PPV_ARGS(&gpuTimeResult)), hr);
#endif

	return hr;
}

HRESULT GraphicManager::CreateGraphicCommand()
{
	// create graphics command queue
	HRESULT hr = S_OK;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	LogIfFailed(mainDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mainGraphicQueue)), hr);

#if defined(GRAPHICTIME)
	LogIfFailed(mainGraphicQueue->GetTimestampFrequency(&gpuFreq), hr);
#endif

	for (int i = 0; i < MAX_FRAME_COUNT; i++)
	{
		LogIfFailed(mainDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(mainGraphicAllocator[i].GetAddressOf())), hr);

		LogIfFailed(mainDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mainGraphicAllocator[i].Get(),
			nullptr,
			IID_PPV_ARGS(mainGraphicList[i].GetAddressOf())), hr);

		// close first because we will call reset at the beginning of render work
		LogIfFailed(mainGraphicList[i]->Close(), hr);
	}

	return hr;
}

HRESULT GraphicManager::CreateGraphicFences()
{
	HRESULT hr = S_OK;

	mainFenceValue = 0;
	for (int i = 0; i < MAX_FRAME_COUNT; i++)
	{
		graphicFences[i] = 0;
	}

	LogIfFailed(mainDevice->CreateFence(mainFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mainGraphicFence)), hr);
	mainFenceValue++;

	mainFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (mainFenceEvent == nullptr)
	{
		hr = S_FALSE;
	}

	// Signal and increment the fence value.
	UINT64 fenceToWaitFor = mainFenceValue;
	LogIfFailed(mainGraphicQueue->Signal(mainGraphicFence.Get(), mainFenceValue), hr);
	mainFenceValue++;

	// Wait until the fence is completed.
	LogIfFailed(mainGraphicFence->SetEventOnCompletion(fenceToWaitFor, mainFenceEvent), hr);
	WaitForSingleObject(mainFenceEvent, INFINITE);

	return hr;
}

bool GraphicManager::CreateGraphicThreads()
{
	bool result = true;

	struct threadwrapper
	{
		static unsigned int WINAPI render(LPVOID lpParameter)
		{
			ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
			GraphicManager::Instance().RenderThread();
			return 0;
		}

		static unsigned int WINAPI worker(LPVOID lpParameter)
		{
			ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
			ForwardRenderingPath::Instance().WorkerThread(parameter->threadIndex);
			return 0;
		}
	};

	// render thread 
	beginRenderThread = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	renderThreadFinish = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	renderThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(
		nullptr,
		0,
		threadwrapper::render,
		nullptr,
		0,
		nullptr));

	result = (beginRenderThread != nullptr) && (renderThreadFinish != nullptr) && (renderThreadHandle != nullptr);

	// worker thread (number of cores - 1)
	for (int i = 0; i < numOfLogicalCores - 1; i++)
	{
		threadParams[i].threadIndex = i;
		beginWorkerThread[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		workerThreadFinish[i] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		workerThreadHandle[i] = reinterpret_cast<HANDLE>(_beginthreadex(
			nullptr,
			0,
			threadwrapper::worker,
			reinterpret_cast<LPVOID>(&threadParams[i]),
			0,
			nullptr));

		result &= (workerThreadHandle[i] != nullptr) && (workerThreadFinish[i] != nullptr) && (beginWorkerThread[i] != nullptr);
	}

	if (!result)
	{
		LogMessage(L"[SqGraphic Error] SqGraphic: Create Render Thread Failed.");
	}

	return result;
}

void GraphicManager::DrawCamera()
{
	GameTimerManager::Instance().gameTime.cullingTime = 0.0;

	vector<Camera> cams = CameraManager::Instance().GetCameras();
	for (size_t i = 0; i < cams.size(); i++)
	{
		// render path
		if (cams[i].GetCameraData().renderingPath == RenderingPathType::Forward)
		{
			ForwardRenderingPath::Instance().CullingWork(cams[i]);
			ForwardRenderingPath::Instance().RenderLoop(cams[i], currFrameIndex);
		}
	}
}

void GraphicManager::WaitForGPU()
{
	if (mainGraphicFence)
	{
		const UINT64 fence = mainFenceValue;
		const UINT64 lastCompletedFence = mainGraphicFence->GetCompletedValue();

		// Signal and increment the fence value.
		mainGraphicQueue->Signal(mainGraphicFence.Get(), mainFenceValue);
		mainFenceValue++;

		// Wait until gpu is finished.
		if (lastCompletedFence < fence)
		{
			mainGraphicFence->SetEventOnCompletion(fence, mainFenceEvent);
			WaitForSingleObject(mainFenceEvent, INFINITE);
		}
	}
}

void GraphicManager::RenderThread()
{
	while (true)
	{
		// wait for main thread notify to draw
		WaitForSingleObject(beginRenderThread, INFINITE);

		// process render thread
#if defined(GRAPHICTIME)
		TIMER_INIT
		TIMER_START
		GameTimerManager::Instance().gameTime.renderThreadTime = 0.0f;
#endif

		DrawCamera();

		// Signal and increment the fence value.
		graphicFences[currFrameIndex] = mainFenceValue;
		mainGraphicQueue->Signal(mainGraphicFence.Get(), mainFenceValue);
		mainFenceValue++;

		SetEvent(renderThreadFinish);

#if defined(GRAPHICTIME)
		TIMER_STOP
		GameTimerManager::Instance().gameTime.renderThreadTime = elapsedTime;
#endif
	}
}

void GraphicManager::WaitForRenderThread()
{
	WaitForSingleObject(renderThreadFinish, INFINITE);
}

ID3D12Device * GraphicManager::GetDevice()
{
	return mainDevice;
}

ID3D12QueryHeap * GraphicManager::GetGpuTimeQuery()
{
	return gpuTimeQuery.Get();
}

ID3D12Resource * GraphicManager::GetGpuTimeResult()
{
	return gpuTimeResult.Get();
}

UINT GraphicManager::GetRtvDesciptorSize()
{
	return rtvDescriptorSize;
}

UINT GraphicManager::GetDsvDesciptorSize()
{
	return dsvDescriptorSize;
}

UINT GraphicManager::GetCbvSrvUavDesciptorSize()
{
	return cbvSrvUavDescriptorSize;
}

FrameResource GraphicManager::GetFrameResource()
{
	FrameResource fr;

	fr.mainGraphicList = mainGraphicList[currFrameIndex].Get();
	fr.mainGraphicAllocator = mainGraphicAllocator[currFrameIndex].Get();
	fr.mainGraphicQueue = mainGraphicQueue.Get();

	return fr;
}

GameTime GraphicManager::GetGameTime()
{
	return GameTimerManager::Instance().gameTime;
}

UINT64 GraphicManager::GetGpuFreq()
{
	return gpuFreq;
}

void GraphicManager::WaitBeginWorkerThread(int _index)
{
	WaitForSingleObject(beginWorkerThread[_index], INFINITE);
}

void GraphicManager::SetBeginWorkerThreadEvent()
{
	for (int i = 0; i < numOfLogicalCores - 1; i++)
	{
		SetEvent(beginWorkerThread[i]);
	}
}

void GraphicManager::ResetWorkerThreadFinish()
{
	for (int i = 0; i < numOfLogicalCores - 1; i++)
	{
		ResetEvent(workerThreadFinish[i]);
	}
}

void GraphicManager::WaitForWorkerThread()
{
	WaitForMultipleObjects(numOfLogicalCores - 1, workerThreadFinish, TRUE, INFINITE);
}

void GraphicManager::SetWorkerThreadFinishEvent(int _index)
{
	SetEvent(workerThreadFinish[_index]);
}
