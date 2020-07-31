#include "Shader.h"

Shader::Shader(wstring _name)
{
	name = _name;
}

void Shader::Release()
{
	vertexShader.Reset();
	pixelShader.Reset();
	domainShader.Reset();
	hullShader.Reset();
	geometryShader.Reset();
	rayGenShader.Reset();
	closestHitShader.Reset();
	missShader.Reset();
	dxcBlob.Reset();

	for (int i = 0; i < MAX_KEYWORD; i++)
	{
		keywords[i] = "";
	}
}

wstring Shader::GetName()
{
	return name;
}

void Shader::SetVS(ComPtr<ID3DBlob> _input, string _entry)
{
	vertexShader = _input;
	entryVS = _entry;
}

void Shader::SetPS(ComPtr<ID3DBlob> _input, string _entry)
{
	pixelShader = _input;
	entryPS = _entry;
}

void Shader::SetHS(ComPtr<ID3DBlob> _input, string _entry)
{
	hullShader = _input;
	entryHS = _entry;
}

void Shader::SetDS(ComPtr<ID3DBlob> _input, string _entry)
{
	domainShader = _input;
	entryDS = _entry;
}

void Shader::SetGS(ComPtr<ID3DBlob> _input, string _entry)
{
	geometryShader = _input;
	entryGS = _entry;
}

void Shader::SetRayGen(ComPtr<IDxcBlob> _input, wstring _entry)
{
	rayGenShader = _input;
	entryRayGen = _entry;
}

void Shader::SetClosestHit(ComPtr<IDxcBlob> _input, wstring _entry)
{
	closestHitShader = _input;
	entryClosest = _entry;
}

void Shader::SetMiss(ComPtr<IDxcBlob> _input, wstring _entry)
{
	missShader = _input;
	entryMiss = _entry;
}

void Shader::SetDxcBlob(ComPtr<IDxcBlobEncoding> _input)
{
	dxcBlob = _input;
}

void Shader::SetRS(ID3D12RootSignature* _rs)
{
	rootSignature = _rs;
}

void Shader::CollectAllKeyword(vector<string> _keywords, D3D_SHADER_MACRO* macro)
{
	for (int i = 0; i < MAX_KEYWORD; i++)
	{
		if (i < (int)_keywords.size())
		{
			keywords[i] = _keywords[i];
		}
		else
		{
			keywords[i] = "";
		}
	}

	if (macro != nullptr)
	{
		keywordUsage = CalcKeywordUsage(macro);
	}
	else
	{
		keywordUsage = 0;
	}
}

void Shader::SetHitGroupName(wstring _hitGroup)
{
	entryHitGroup = _hitGroup;
}

void Shader::SetPayloadSize(UINT _size)
{
	payloadSize = _size;
}

ComPtr<ID3DBlob> Shader::GetVS()
{
	return vertexShader;
}

ComPtr<ID3DBlob> Shader::GetPS()
{
	return pixelShader;
}

ComPtr<ID3DBlob> Shader::GetDS()
{
	return domainShader;
}

ComPtr<ID3DBlob> Shader::GetHS()
{
	return hullShader;
}

ComPtr<ID3DBlob> Shader::GetGS()
{
	return geometryShader;
}

ComPtr<IDxcBlob> Shader::GetRayGen()
{
	return rayGenShader;
}

ComPtr<IDxcBlob> Shader::GetClosestHit()
{
	return closestHitShader;
}

ComPtr<IDxcBlob> Shader::GetMiss()
{
	return missShader;
}

ID3D12RootSignature* Shader::GetRS()
{
	return rootSignature;
}

IDxcBlobEncoding* Shader::GetDxcBlob()
{
	return dxcBlob.Get();
}

bool Shader::IsSameKeyword(D3D_SHADER_MACRO* macro)
{
	if (macro == nullptr)
	{
		// no keyword usage at all
		if (keywordUsage == 0)
		{
			return true;
		}

		// can't continue
		return false;
	}

	if (CalcKeywordUsage(macro) == keywordUsage)
	{
		return true;
	}

	return false;
}

wstring Shader::GetRayGenName()
{
	return entryRayGen;
}

wstring Shader::GetClosestName()
{
	return entryClosest;
}

wstring Shader::GetMissName()
{
	return entryMiss;
}

wstring Shader::GetHitGroup()
{
	return entryHitGroup;
}

UINT Shader::GetPayloadSize()
{
	return payloadSize;
}

int Shader::CalcKeywordUsage(D3D_SHADER_MACRO* macro)
{
	int usageBit = 0;
	D3D_SHADER_MACRO* p = &macro[0];
	while (p->Name != NULL)
	{
		for (int i = 0; i < MAX_KEYWORD; i++)
		{
			if (p->Name == keywords[i])
			{
				usageBit |= (1 << i);
			}
		}

		p++;
	}

	return usageBit;
}
