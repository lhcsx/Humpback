// (c) Li Hongcheng
// 2022/01/26

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Humpback 
{

	struct Mesh
	{
	public:

		Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

		// Data about the buffers.
		unsigned int vertexByteStride = 0;
		unsigned int vertexBufferByteSize = 0;
		DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
		
		unsigned int indexBufferByteSize = 0;

		std::string Name;

		std::unordered_map<std::string, SubMesh> drawArgs;
	};

	struct SubMesh
	{
	public:
		
		unsigned int indexCount;
		unsigned int startIndexLocation;
		int baseVertexLocation;
	};
}

