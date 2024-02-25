// (c) Li Hongcheng
// 2022-11-19

#pragma once


#include <string>
#include <DirectXMath.h>

#include "HMathHelper.h"

namespace Humpback
{
	extern const int FRAME_RESOURCE_COUNT;

	struct MaterialConstants
	{
		DirectX::XMFLOAT4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4X4 matTransform = HMathHelper::Identity4x4();

		unsigned int diffuseMapIndex = 0;
		unsigned int normalMapIndex = 0;
		unsigned int metallicSmothnessMapIndex = 0;
		unsigned int pad1;
	};


	class Material
	{
	public:

		std::string name;
		
		int matCBIdx = 0;
		int diffuseSrvHeapIndex = 0;
		int normalSrvHeapIndex = 0;
		int metallicSmothnessSrvHeapIndex = 0;
		
		int numFramesDirty = FRAME_RESOURCE_COUNT;

		DirectX::XMFLOAT4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };

		DirectX::XMFLOAT4X4 matTransform = HMathHelper::Identity4x4();
	};
}