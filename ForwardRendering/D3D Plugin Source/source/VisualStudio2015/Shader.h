#pragma once
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include "stdafx.h"
#include <vector>
#include <unordered_map>
#include <dxcapi.h>

class Shader
{
public:
	Shader(wstring _name);

	void Release();
	wstring GetName();

	void SetVS(ComPtr<ID3DBlob> _input, string _entry);
	void SetPS(ComPtr<ID3DBlob> _input, string _entry);
	void SetDS(ComPtr<ID3DBlob> _input, string _entry);
	void SetHS(ComPtr<ID3DBlob> _input, string _entry);
	void SetGS(ComPtr<ID3DBlob> _input, string _entry);
	void SetRayGen(ComPtr<IDxcBlob> _input, string _entry);
	void SetClosestHit(ComPtr<IDxcBlob> _input, string _entry);
	void SetMiss(ComPtr<IDxcBlob> _input, string _entry);
	void SetRS(ID3D12RootSignature* _rs);
	void CollectAllKeyword(vector<string> _keywords, D3D_SHADER_MACRO* macro);

	ComPtr<ID3DBlob> GetVS();
	ComPtr<ID3DBlob> GetPS();
	ComPtr<ID3DBlob> GetDS();
	ComPtr<ID3DBlob> GetHS();
	ComPtr<ID3DBlob> GetGS();
	ComPtr<IDxcBlob> GetRayGen();
	ComPtr<IDxcBlob> GetClosestHit();
	ComPtr<IDxcBlob> GetMiss();
	ID3D12RootSignature* GetRS();
	bool IsSameKeyword(D3D_SHADER_MACRO *macro);

	string GetRayGenName();
	string GetClosestName();
	string GetMissName();

private:
	int CalcKeywordUsage(D3D_SHADER_MACRO* macro);

	static const int MAX_KEYWORD = 32;
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	ComPtr<ID3DBlob> domainShader;
	ComPtr<ID3DBlob> hullShader;
	ComPtr<ID3DBlob> geometryShader;
	ComPtr<IDxcBlob> rayGenShader;
	ComPtr<IDxcBlob> closestHitShader;
	ComPtr<IDxcBlob> missShader;

	string entryVS;
	string entryPS;
	string entryHS;
	string entryDS;
	string entryGS;
	string entryRS;
	string entryRayGen;
	string entryClosest;
	string entryMiss;

	// rs will created by Shader Manager
	ID3D12RootSignature* rootSignature;

	wstring name = L"";
	int keywordUsage = 0;	// support up to 32 keywords per shader
	string keywords[MAX_KEYWORD];
};