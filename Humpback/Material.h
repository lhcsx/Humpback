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
		DirectX::XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
		float roughness = 0.25f;

		DirectX::XMFLOAT4X4 matTransform = HMathHelper::Identity4x4();

		unsigned int diffuseMapIndex = 0;
		unsigned int normalMapIndex = 0;
		unsigned int pad1;
		unsigned int pad2;
	};


	class Material
	{
	public:

		std::string name;
		
		int matCBIdx = -1;
		int diffuseSrvHeapIndex = -1;
		int normalSrvHeapIndex = -1;
		
		int numFramesDirty = FRAME_RESOURCE_COUNT;

		DirectX::XMFLOAT4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
		float roughness = 0.25f;

		DirectX::XMFLOAT4X4 matTransform = HMathHelper::Identity4x4();
	};
}