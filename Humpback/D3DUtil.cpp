// (c) Li Hongcheng
// 2022-04-04


#include "D3DUtil.h"
#include "d3dx12.h"


using Microsoft::WRL::ComPtr;


namespace Humpback 
{
	void ThrowIfFailed(HRESULT hr);
	
	Microsoft::WRL::ComPtr<ID3D12Resource> D3DUtil::CreateDefaultBuffer(
		ID3D12Device* device, 
		ID3D12GraphicsCommandList* commandList, 
		const void* initData, 
		uint64_t byteSize, 
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
	{
		ComPtr<ID3D12Resource> defaultBuffer;

		// Create the actual default buffer resource.
		auto actualDefaultProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto actualDefaultBuffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&actualDefaultProperties,
			D3D12_HEAP_FLAG_NONE,
			&actualDefaultBuffer,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

		// Inorder to copy cpu memory data into our default buffer, 
		// we need to create a intermdiate upload heap.
		auto tempUploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto tempUploadBuffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&tempUploadProperties,
			D3D12_HEAP_FLAG_NONE,
			&tempUploadBuffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


		// Describe the data we want to copy into the default buffer.
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = initData;
		subResourceData.RowPitch = byteSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
		// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
		// the intermediate upload heap data will be copied to mBuffer.
		auto trans1 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &trans1);
		auto trans2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		UpdateSubresources<1>(commandList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
		commandList->ResourceBarrier(1, &trans2);

		// Note: uploadBuffer has to be kept alive after the above function calls because
		// the command list has not been executed yet that performs the actual copy.
		// The caller can Release the uploadBuffer after it knows the copy has been executed.

		return defaultBuffer;
	}
}

