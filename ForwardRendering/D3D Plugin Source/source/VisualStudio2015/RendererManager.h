#pragma once
#include "Camera.h"
#include "Renderer.h"
#include "Light.h"
using namespace std;
#include <map>
#include "UploadBuffer.h"
#include "GraphicManager.h"

struct SqInstanceData
{
	XMFLOAT4X4 world;
	XMFLOAT4X4 invWorld;
};

struct QueueRenderer
{
	Renderer* cache;
	int submeshIndex;
	int materialID;
	float zDistanceToCam;
};

struct InstanceRenderer
{
public:
	bool operator==(InstanceRenderer& _in)
	{
		// instance rendering needs the same mesh and material
		return _in.cache->GetMesh()->GetInstanceID() == cache->GetMesh()->GetInstanceID()
			&& _in.submeshIndex == submeshIndex
			&& _in.materialID == materialID;
	}

	InstanceRenderer()
	{
		cache = nullptr;
		submeshIndex = -1;
		materialID = -1;
		maxCapacity = 0;
	}

	void CreateInstanceData()
	{
		for (int i = 0; i < MAX_FRAME_COUNT; i++)
		{
			instanceDataGPU[i] = make_shared<UploadBuffer<SqInstanceData>>(GraphicManager::Instance().GetDevice(), maxCapacity, false);
		}
	}

	void Release()
	{
		instanceDataCPU.clear();
		for (int i = 0; i < MAX_FRAME_COUNT; i++)
		{
			instanceDataGPU[i].reset();
		}
	}

	void AddInstanceData(SqInstanceData _data, float _zDist)
	{
		instanceDataCPU.push_back(_data);
		zDistToCamTotal += _zDist;
	}

	void FinishCollectInstance()
	{
		if ((int)instanceDataCPU.size() == 0)
		{
			return;
		}
		zDistToCamTotal = zDistToCamTotal / (int)instanceDataCPU.size();
	}

	void ClearInstanceData()
	{
		instanceDataCPU.clear();
		zDistToCamTotal = 0;
	}

	int GetInstanceCount()
	{
		return (int)instanceDataCPU.size();
	}

	void UploadInstanceData(int _frameIdx)
	{
		for (int i = 0; i < (int)instanceDataCPU.size() && i < maxCapacity; i++)
		{
			instanceDataGPU[_frameIdx]->CopyData(i, instanceDataCPU[i]);
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetInstanceDataGPU(int _frameIdx)
	{
		return instanceDataGPU[_frameIdx]->Resource()->GetGPUVirtualAddress();
	}

	void CountCapacity()
	{
		maxCapacity++;
	}

	Renderer* cache;
	int submeshIndex;
	int materialID;
	float zDistToCamTotal;

private:
	int maxCapacity;
	vector<SqInstanceData> instanceDataCPU;
	shared_ptr<UploadBuffer<SqInstanceData>> instanceDataGPU[MAX_FRAME_COUNT];
};

inline bool FrontToBackRender(QueueRenderer const& i, QueueRenderer const& j)
{
	// sort from low distance to high distance (front to back)
	return i.zDistanceToCam < j.zDistanceToCam;
}

inline bool BackToFrontRender(QueueRenderer const& i, QueueRenderer const& j)
{
	// sort from high distance to low distance (back to front)
	return i.zDistanceToCam > j.zDistanceToCam;
}

inline bool MaterialIdSort(InstanceRenderer const& i, InstanceRenderer const& j)
{
	return i.materialID < j.materialID;
}

inline bool FrontToBackInstance(InstanceRenderer const& i, InstanceRenderer const& j)
{
	// sort from low distance to high distance (front to back)
	return i.zDistToCamTotal < j.zDistToCamTotal;
}

class RendererManager
{
public:
	RendererManager(const RendererManager&) = delete;
	RendererManager(RendererManager&&) = delete;
	RendererManager& operator=(const RendererManager&) = delete;
	RendererManager& operator=(RendererManager&&) = delete;

	static RendererManager& Instance()
	{
		static RendererManager instance;
		return instance;
	}

	RendererManager() {}
	~RendererManager() {}

	void Init();
	int AddRenderer(int _instanceID, int _meshInstanceID, bool _isDynamic);
	void AddCreatedMaterial(int _instanceID, Material *_mat);
	void InitInstanceRendering();
	void UpdateLocalBound(int _id, float _x, float _y, float _z, float _ex, float _ey, float _ez);
	void UploadObjectConstant(int _frameIdx, int _threadIndex, int _numThreads);
	void UploadInstanceData(int _frameIdx, int _threadIndex, int _numThreads);
	void SetWorldMatrix(int _id, XMFLOAT4X4 _world);
	void Release();
	void SetNativeRendererActive(int _id, bool _active);
	void SortWork(Camera* _camera);
	void FrustumCulling(Camera* _camera, int _threadIdx);

	bool ValidRenderer(int _index, vector<QueueRenderer> &_renderers);
	bool ValidRenderer(int _index, vector<InstanceRenderer>& _renderers);

	vector<shared_ptr<Renderer>> &GetRenderers();
	map<int, vector<QueueRenderer>>& GetQueueRenderers();
	map<int, vector<InstanceRenderer>>& GetInstanceRenderers();

private:
	static const int OPAQUE_CAPACITY = 5000;
	static const int CUTOFF_CAPACITY = 2500;
	static const int TRANSPARENT_CAPACITY = 500;

	void ClearQueueRenderer();
	void ClearInstanceRendererData();
	void AddToQueueRenderer(Renderer* _renderer, Camera* _camera);
	void AddToInstanceRenderer(Renderer* _renderer, Camera* _camera);
	int FindInstanceRenderer(int _queue, InstanceRenderer _ir);

	vector<shared_ptr<Renderer>> renderers;
	map<int, vector<QueueRenderer>> queuedRenderers;
	map<int, vector<InstanceRenderer>> instanceRenderers;
};