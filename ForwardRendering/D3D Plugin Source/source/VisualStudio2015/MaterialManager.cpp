#include "MaterialManager.h"
#include "d3dx12.h"
#include "MeshManager.h"
#include "ShaderManager.h"
#include "CameraManager.h"

void MaterialManager::Init()
{
	blendTable[0] = D3D12_BLEND_ZERO;
	blendTable[1] = D3D12_BLEND_ONE;
	blendTable[2] = D3D12_BLEND_DEST_COLOR;
	blendTable[3] = D3D12_BLEND_SRC_COLOR;
	blendTable[4] = D3D12_BLEND_INV_DEST_COLOR;
	blendTable[5] = D3D12_BLEND_SRC_ALPHA;
	blendTable[6] = D3D12_BLEND_INV_SRC_COLOR;
	blendTable[7] = D3D12_BLEND_DEST_ALPHA;
	blendTable[8] = D3D12_BLEND_INV_DEST_ALPHA;
	blendTable[9] = D3D12_BLEND_SRC_ALPHA_SAT;
	blendTable[10] = D3D12_BLEND_INV_SRC_ALPHA;
}

Material MaterialManager::CreateMaterialFromShader(Shader* _shader, Camera* _camera, D3D12_FILL_MODE _fillMode, D3D12_CULL_MODE _cullMode
	, int _srcBlend, int _dstBlend, D3D12_COMPARISON_FUNC _depthFunc, bool _zWrite)
{
	auto desc = CollectPsoDesc(_shader, _camera, _fillMode, _cullMode, _srcBlend, _dstBlend, _depthFunc, _zWrite);

	Material result;
	result.CreatePsoFromDesc(desc);
	return result;
}

Material MaterialManager::CreateMaterialDepthOnly(Shader* _shader, D3D12_FILL_MODE _fillMode, D3D12_CULL_MODE _cullMode, int _srcBlend, int _dstBlend, D3D12_COMPARISON_FUNC _depthFunc, bool _zWrite)
{
	auto desc = CollectPsoDepth(_shader, _fillMode, _cullMode, _srcBlend, _dstBlend, _depthFunc, _zWrite);

	Material result;
	result.CreatePsoFromDesc(desc);
	return result;
}

Material MaterialManager::CreateMaterialPost(Shader* _shader, bool _enableDepth, int _numRT, DXGI_FORMAT* _rtDesc, DXGI_FORMAT _dsDesc)
{
	// create pso
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.pRootSignature = _shader->GetRootSignatureRef().Get();
	desc.VS.BytecodeLength = _shader->GetVS()->GetBufferSize();
	desc.VS.pShaderBytecode = reinterpret_cast<BYTE*>(_shader->GetVS()->GetBufferPointer());
	desc.PS.BytecodeLength = _shader->GetPS()->GetBufferSize();
	desc.PS.pShaderBytecode = reinterpret_cast<BYTE*>(_shader->GetPS()->GetBufferPointer());
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.RasterizerState.FrontCounterClockwise = FALSE;
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	desc.DepthStencilState.DepthEnable = _enableDepth;

	desc.InputLayout.pInputElementDescs = nullptr;
	desc.InputLayout.NumElements = 0;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = _numRT;

	for (int i = 0; i < _numRT; i++)
	{
		desc.RTVFormats[i] = _rtDesc[i];
	}

	desc.DSVFormat = _dsDesc;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	Material result;
	result.CreatePsoFromDesc(desc);
	return result;
}

Material* MaterialManager::AddMaterial(int _matInstanceId, int _renderQueue, int _cullMode, int _srcBlend, int _dstBlend, char* _nativeShader, int _numMacro, char** _macro)
{
	if (materialTable.find(_matInstanceId) != materialTable.end())
	{
		return materialTable[_matInstanceId].get();
	}

	if (_nativeShader == nullptr)
	{
		materialTable[_matInstanceId] = make_unique<Material>();
	}
	else
	{
		Shader* forwardShader = nullptr;

		if (_numMacro == 0)
		{
			forwardShader = ShaderManager::Instance().CompileShader(AnsiToWString(_nativeShader));
		}
		else
		{
			D3D_SHADER_MACRO* macro = new D3D_SHADER_MACRO[_numMacro + 1];
			for (int i = 0; i < _numMacro; i++)
			{
				macro[i].Name = _macro[i];
				macro[i].Definition = "1";
			}
			macro[_numMacro].Name = NULL;
			macro[_numMacro].Definition = NULL;

			forwardShader = ShaderManager::Instance().CompileShader(AnsiToWString(_nativeShader), macro);
			delete[] macro;
		}

		auto c = CameraManager::Instance().GetCamera();
		if (forwardShader != nullptr)
		{
			materialTable[_matInstanceId] = make_unique<Material>(CreateMaterialFromShader(forwardShader, c, D3D12_FILL_MODE_SOLID, (D3D12_CULL_MODE)(_cullMode + 1)
				, _srcBlend, _dstBlend, (_renderQueue <= RenderQueue::OpaqueLast) ? D3D12_COMPARISON_FUNC_EQUAL : D3D12_COMPARISON_FUNC_GREATER, false));
		}
		else
		{
			materialTable[_matInstanceId] = make_unique<Material>(CreateMaterialFromShader(c->GetFallbackShader(), c, D3D12_FILL_MODE_SOLID, (D3D12_CULL_MODE)(_cullMode + 1)
				, _srcBlend, _dstBlend, (_renderQueue <= RenderQueue::OpaqueLast) ? D3D12_COMPARISON_FUNC_EQUAL : D3D12_COMPARISON_FUNC_GREATER, false));
		}
	}

	materialTable[_matInstanceId]->SetRenderQueue(_renderQueue);
	materialTable[_matInstanceId]->SetCullMode(_cullMode);
	materialTable[_matInstanceId]->SetBlendMode(_srcBlend, _dstBlend);

	return materialTable[_matInstanceId].get();
}

void MaterialManager::ResetNativeMaterial(Camera* _camera)
{
	for (auto& m : materialTable)
	{
		auto desc = m.second->GetPsoDesc();
		desc.SampleDesc.Count = _camera->GetMsaaCount();
		desc.SampleDesc.Quality = _camera->GetMsaaQuailty();

		m.second->CreatePsoFromDesc(desc);
	}
}

void MaterialManager::Release()
{
	for (auto& m : materialTable)
	{
		m.second->Release();
	}
	materialTable.clear();
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC MaterialManager::CollectPsoDesc(Shader* _shader, Camera* _camera, D3D12_FILL_MODE _fillMode, D3D12_CULL_MODE _cullMode,
	int _srcBlend, int _dstBlend, D3D12_COMPARISON_FUNC _depthFunc, bool _zWrite)
{
	// create pso
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.pRootSignature = _shader->GetRootSignatureRef().Get();
	desc.VS.BytecodeLength = _shader->GetVS()->GetBufferSize();
	desc.VS.pShaderBytecode = reinterpret_cast<BYTE*>(_shader->GetVS()->GetBufferPointer());
	desc.PS.BytecodeLength = _shader->GetPS()->GetBufferSize();
	desc.PS.pShaderBytecode = reinterpret_cast<BYTE*>(_shader->GetPS()->GetBufferPointer());

	// feed blend according to input
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.BlendState.IndependentBlendEnable = (_camera->GetNumOfRT() > 1);
	for (int i = 0; i < _camera->GetNumOfRT(); i++)
	{
		// if it is one/zero, doesn't need blend
		desc.BlendState.RenderTarget[i].BlendEnable = (_srcBlend == 1 && _dstBlend == 0) ? FALSE : TRUE;
		desc.BlendState.RenderTarget[i].SrcBlend = blendTable[_srcBlend];
		desc.BlendState.RenderTarget[i].DestBlend = blendTable[_dstBlend];
	}

	desc.SampleMask = UINT_MAX;
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.RasterizerState.FrontCounterClockwise = true;
	desc.RasterizerState.FillMode = _fillMode;
	desc.RasterizerState.CullMode = _cullMode;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.DepthStencilState.DepthFunc = _depthFunc;
	desc.DepthStencilState.DepthWriteMask = (_zWrite) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

	desc.InputLayout.pInputElementDescs = MeshManager::Instance().GetDefaultInputLayout();
	desc.InputLayout.NumElements = MeshManager::Instance().GetDefaultInputLayoutSize();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = _camera->GetNumOfRT();

	auto rtDesc = _camera->GetColorRTDesc();
	for (int i = 0; i < _camera->GetNumOfRT(); i++)
	{
		desc.RTVFormats[i] = rtDesc[i].Format;
	}

	desc.DSVFormat = _camera->GetDepthDesc().Format;
	desc.SampleDesc.Count = _camera->GetMsaaCount();
	desc.SampleDesc.Quality = _camera->GetMsaaQuailty();

	return desc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC MaterialManager::CollectPsoDepth(Shader* _shader, D3D12_FILL_MODE _fillMode, D3D12_CULL_MODE _cullMode, int _srcBlend, int _dstBlend, D3D12_COMPARISON_FUNC _depthFunc, bool _zWrite)
{
	// create pso
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.pRootSignature = _shader->GetRootSignatureRef().Get();
	desc.VS.BytecodeLength = _shader->GetVS()->GetBufferSize();
	desc.VS.pShaderBytecode = reinterpret_cast<BYTE*>(_shader->GetVS()->GetBufferPointer());
	desc.PS.BytecodeLength = _shader->GetPS()->GetBufferSize();
	desc.PS.pShaderBytecode = reinterpret_cast<BYTE*>(_shader->GetPS()->GetBufferPointer());

	// feed blend according to input
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	desc.SampleMask = UINT_MAX;
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.RasterizerState.FrontCounterClockwise = true;
	desc.RasterizerState.FillMode = _fillMode;
	desc.RasterizerState.CullMode = _cullMode;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.DepthStencilState.DepthFunc = _depthFunc;
	desc.DepthStencilState.DepthWriteMask = (_zWrite) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

	desc.InputLayout.pInputElementDescs = MeshManager::Instance().GetDefaultInputLayout();
	desc.InputLayout.NumElements = MeshManager::Instance().GetDefaultInputLayoutSize();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 0;

	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	return desc;
}
