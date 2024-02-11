// (c) Li Hongcheng
// 2022-04-04


#include <d3dcompiler.h>
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
		auto uploadBufferProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadBufferProperties,
			D3D12_HEAP_FLAG_NONE,
			&uploadBufferDesc,
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


	ComPtr<ID3DBlob> D3DUtil::CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string_view target)
	{
		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		HRESULT hr = S_OK;

		ComPtr<ID3DBlob> byteCode = nullptr;
		ComPtr<ID3DBlob> errors;
		hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entrypoint.c_str(), target.data(), compileFlags, 0, &byteCode, &errors);

		if (errors != nullptr)
			OutputDebugStringA((char*)errors->GetBufferPointer());

		ThrowIfFailed(hr);

		return byteCode;
	}

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> D3DUtil::GetCommonStaticSamplers()
	{
		const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
			1, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			2, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
			3, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
			4, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
			0.0f,                             // mipLODBias
			8);                               // maxAnisotropy

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
			5, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
			0.0f,                              // mipLODBias
			8);                                // maxAnisotropy

		const CD3DX12_STATIC_SAMPLER_DESC shadow(
			6, // shaderRegister
			D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
			0.0f,                               // mipLODBias
			16,                                 // maxAnisotropy
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

		return {
			pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap, anisotropicClamp, shadow };
	}

	HRESULT D3DUtil::CreateD3DResources12(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, uint32_t resDim, size_t width, size_t height, size_t depth, size_t mipCount, size_t arraySize, DXGI_FORMAT format, bool forceSRGB, bool isCubeMap, D3D12_SUBRESOURCE_DATA* initData, ComPtr<ID3D12Resource>& texture, ComPtr<ID3D12Resource>& textureUploadHeap)
	{
		{
			if (device == nullptr)
				return E_POINTER;

			// TODO
			// Uncomment and define MakeSRGB.
			//if (forceSRGB)
			//	format = MakeSRGB(format);

			HRESULT hr = E_FAIL;
			switch (resDim)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			{
				D3D12_RESOURCE_DESC texDesc;
				ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
				texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				texDesc.Alignment = 0;
				texDesc.Width = width;
				texDesc.Height = (uint32_t)height;
				texDesc.DepthOrArraySize = (depth > 1) ? (uint16_t)depth : (uint16_t)arraySize;
				texDesc.MipLevels = (uint16_t)mipCount;
				texDesc.Format = format;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

				auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				hr = device->CreateCommittedResource(
					&heapProperties,
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&texture)
				);

				if (FAILED(hr))
				{
					texture = nullptr;
					return hr;
				}
				else
				{
					const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
					const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, num2DSubresources);

					auto heapProperties1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
					auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
					hr = device->CreateCommittedResource(
						&heapProperties1,
						D3D12_HEAP_FLAG_NONE,
						&resDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&textureUploadHeap));
					if (FAILED(hr))
					{
						texture = nullptr;
						return hr;
					}
					else
					{
						auto resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
							D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
						cmdList->ResourceBarrier(1, &resBarrier);

						// Use Heap-allocating UpdateSubresources implementation for variable number of subresources (which is the case for textures).
						UpdateSubresources(cmdList, texture.Get(), textureUploadHeap.Get(), 0, 0, num2DSubresources, initData);

						auto resBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
							D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
						cmdList->ResourceBarrier(1, &resBarrier1);
					}
				}
			} break;
			}

			return hr;
		}
	}
}

