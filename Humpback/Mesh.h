// (c) Li Hongcheng
// 2022-09-04


#pragma once


#include <unordered_map>
#include <wrl.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <d3d12.h>


namespace Humpback 
{
	struct SubMesh
	{
	public:

		unsigned int indexCount;
		unsigned int startIndexLocation;
		int baseVertexLocation;
	};

	class Mesh
	{
	public:

		Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

		// Data about the buffers.
		unsigned int vertexByteStride = 0;
		unsigned int vertexBufferByteSize = 0;
		DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;

		unsigned int indexBufferByteSize = 0;

		std::string Name;

		std::unordered_map<std::string, SubMesh> drawArgs;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
		{
			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
			vbv.StrideInBytes = vertexByteStride;
			vbv.SizeInBytes = vertexBufferByteSize;
			return vbv;
		}

		D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
		{
			D3D12_INDEX_BUFFER_VIEW ibv;
			ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
			ibv.Format = indexFormat;
			ibv.SizeInBytes = indexBufferByteSize;
			return ibv;
		}
	};
}


