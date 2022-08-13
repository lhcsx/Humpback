// (c) Li Hongcheng
// 2022/01/08

#pragma once

#include "D3DUtil.h"
#include "d3dx12.h"

namespace Humpback 
{
	inline void ThrowIfFailed(HRESULT hr);

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, unsigned int elementCount, bool isConstantBuffer) :
		m_isConstantBuffer(isConstantBuffer)
	{
		m_elementByteSize = sizeof(T);

		// Constant buffer elements need to be multiples of 256 bytes.
		// This is because the hardware can only view constant data 
		// at m*256 byte offsets and of n*256 byte lengths. 
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT   SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
		{
			m_elementByteSize = D3DUtil::CalConstantBufferByteSize(m_elementByteSize);
		}

		auto uploadHeapPro = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * elementCount);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapPro,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_uploadBuffer)));

		ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if (m_uploadBuffer != nullptr)
		{
			m_uploadBuffer->Unmap(0, nullptr);
		}
		
		m_mappedData = nullptr;
	}

	ID3D12Resource* Resource() const 
	{
		return m_uploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&m_mappedData[elementIndex * m_elementByteSize], &data, sizeof(T));
	}


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer = nullptr;

	byte* m_mappedData = nullptr;

	unsigned int m_elementByteSize = 0;
	bool m_isConstantBuffer = false;
};

}

