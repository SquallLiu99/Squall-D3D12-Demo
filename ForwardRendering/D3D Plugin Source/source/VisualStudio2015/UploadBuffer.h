#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include "stdafx.h"
#include <wrl.h>
using namespace Microsoft::WRL;

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) : 
        mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = sizeof(T);

        // Constant buffer elements need to be multiples of 256 bytes.
        // This is because the hardware can only view constant data 
        // at m*256 byte offsets and of n*256 byte lengths. 
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // multiple of 256
        // UINT   SizeInBytes;   // multiple of 256
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
        if(isConstantBuffer)
            mElementByteSize = (sizeof(T) + 255) & ~255;

		LogIfFailedWithoutHR(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

		LogIfFailedWithoutHR(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if(mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
    }

    void CopyDataAll(T* data)
    {
        memcpy(&mMappedData[0], data, mElementByteSize);
    }

private:
    ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};

class UploadBufferAny
{
public:
    UploadBufferAny(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, UINT _byteSize) :
        mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = _byteSize;

        // Constant buffer elements need to be multiples of 256 bytes.
        // This is because the hardware can only view constant data 
        // at m*256 byte offsets and of n*256 byte lengths. 
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // multiple of 256
        // UINT   SizeInBytes;   // multiple of 256
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
        if (isConstantBuffer)
            mElementByteSize = (_byteSize + 255) & ~255;

        LogIfFailedWithoutHR(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        LogIfFailedWithoutHR(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
    }

    UploadBufferAny(const UploadBufferAny& rhs) = delete;
    UploadBufferAny& operator=(const UploadBufferAny& rhs) = delete;
    ~UploadBufferAny()
    {
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    UINT Stride()const
    {
        return mElementByteSize;
    }

    void CopyData(int elementIndex, const void* data)
    {
        memcpy(&mMappedData[elementIndex * mElementByteSize], data, mElementByteSize);
    }

    void CopyDataByteSize(int elementIndex, const void* data, UINT _byteSize)
    {
        // copy partial data to dest
        memcpy(&mMappedData[elementIndex * mElementByteSize], data, _byteSize);
    }

    void CopyDataOffset(int elementIndex, const void* data, UINT _destOffet, UINT _sizeOffset)
    {
        // copy data with offset
        memcpy(&mMappedData[elementIndex * mElementByteSize] + _destOffet, data, mElementByteSize + _sizeOffset);
    }

private:
    ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};

inline UINT CalcConstantBufferByteSize(UINT _size)
{
    return(_size + 255) & ~255;
}