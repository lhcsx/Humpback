// (c) Li Hongcheng
// 2022-12-15


#pragma once


#include <string>
#include <wrl.h>
#include <d3d12.h>

namespace Humpback
{
	class Texture
	{
	public:

		std::string name;
		std::string type;

		std::wstring filePath;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap = nullptr;
	};
}