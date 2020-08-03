#include "Material.h"
#include "GraphicManager.h"

bool Material::CreatePsoFromDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC _desc)
{
	HRESULT hr = S_OK;

	psoDesc = _desc;
	pso.Reset();

	LogIfFailed(GraphicManager::Instance().GetDevice()->CreateGraphicsPipelineState(&_desc, IID_PPV_ARGS(&pso)), hr);
	validMaterial = SUCCEEDED(hr);

	isDirty = true;

	return validMaterial;
}

void Material::CreateDxcPso(ComPtr<ID3D12StateObject> _pso, Shader* _shader)
{
	dxcPso = _pso;
	validMaterial = (dxcPso != nullptr);

	if (validMaterial)
	{
		// create shader table here
		ID3D12Device* device = GraphicManager::Instance().GetDevice();
		UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		rayGenShaderTable = make_shared<UploadBufferAny>(device, 1, false, shaderIdentifierSize);
		missShaderTable = make_shared<UploadBufferAny>(device, 1, false, shaderIdentifierSize);
		hitGroupTable = make_shared<UploadBufferAny>(device, 1, false, shaderIdentifierSize);

		// Get shader identifiers.
		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		LogIfFailedWithoutHR(dxcPso.As(&stateObjectProperties));

		auto rtse = _shader->GetRTSEntry();
		void* rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rtse.entryRayGen.c_str());
		void* missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rtse.entryMiss.c_str());
		void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rtse.entryHitGroup.c_str());

		rayGenShaderTable->CopyData(0, rayGenShaderIdentifier);
		missShaderTable->CopyData(0, missShaderIdentifier);
		hitGroupTable->CopyData(0, hitGroupShaderIdentifier);

		psoDesc.pRootSignature = _shader->GetRS();
	}
}

void Material::AddMaterialConstant(UINT _byteSize, void* _data)
{
	for (int i = 0; i < MAX_FRAME_COUNT; i++)
	{
		materialConstant[i] = make_shared<UploadBufferAny>(GraphicManager::Instance().GetDevice(), 1, true, _byteSize);
		materialConstant[i]->CopyData(0, _data);
	}
}

void Material::Release()
{
	pso.Reset();
	dxcPso.Reset();
	for (int i = 0; i < MAX_FRAME_COUNT; i++)
	{
		materialConstant[i].reset();
	}

	rayGenShaderTable.reset();
	missShaderTable.reset();
	hitGroupTable.reset();
}

void Material::SetRenderQueue(int _queue)
{
	renderQueue = _queue;
}

void Material::SetCullMode(int _mode)
{
	cullMode = (CullMode)_mode;
}

void Material::SetBlendMode(int _srcBlend, int _dstBlend)
{
	srcBlend = _srcBlend;
	dstBlend = _dstBlend;
}

ID3D12PipelineState * Material::GetPSO()
{
	return pso.Get();
}

ID3D12StateObject* Material::GetDxcPSO()
{
	return dxcPso.Get();
}

ID3D12RootSignature* Material::GetRootSignature()
{
	return psoDesc.pRootSignature;
}

int Material::GetRenderQueue()
{
	return renderQueue;
}

CullMode Material::GetCullMode()
{
	return cullMode;
}

D3D12_GPU_VIRTUAL_ADDRESS Material::GetMaterialConstantGPU(int _index)
{
	return materialConstant[_index]->Resource()->GetGPUVirtualAddress();
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC Material::GetPsoDesc()
{
	return psoDesc;
}

D3D12_DISPATCH_RAYS_DESC Material::GetDispatchRayDesc(UINT _width, UINT _height)
{
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

	// Since each shader table has only one shader record, the stride is same as the size.
	dispatchDesc.HitGroupTable.StartAddress = hitGroupTable->Resource()->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = hitGroupTable->Resource()->GetDesc().Width;
	dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
	dispatchDesc.MissShaderTable.StartAddress = missShaderTable->Resource()->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = missShaderTable->Resource()->GetDesc().Width;
	dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
	dispatchDesc.RayGenerationShaderRecord.StartAddress = rayGenShaderTable->Resource()->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGenShaderTable->Resource()->GetDesc().Width;
	dispatchDesc.Width = _width;
	dispatchDesc.Height = _height;
	dispatchDesc.Depth = 1;

	return dispatchDesc;
}
