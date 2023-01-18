// (c) Li Hongcheng
// 2022-08-26


#pragma once


#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

#include "HMathHelper.h"
#include "Mesh.h"
#include "Material.h"


using DirectX::XMFLOAT4X4;

namespace Humpback
{

	extern const int FRAME_RESOURCE_COUNT;

	struct InstanceData
	{
		DirectX::XMFLOAT4X4 worldMatrix = HMathHelper::Identity4x4();
		unsigned int materialIndex = 0;
		unsigned int pad0 = 0;
		unsigned int pad1 = 0;
		unsigned int pad2 = 0;
	};

	class RenderableObject
	{
	public:
		RenderableObject() = default;


		XMFLOAT4X4 worldM = HMathHelper::Identity4x4();

		XMFLOAT4X4 texTrans = HMathHelper::Identity4x4();

		int NumFramesDirty = FRAME_RESOURCE_COUNT;

		Mesh* mesh = nullptr;
		Material* material = nullptr;
		
		DirectX::BoundingBox aabb;

		D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		std::vector<InstanceData> instances;

		unsigned int cbIndex = 0;

		unsigned int instanceCount = 0;
		unsigned int indexCount = 0;
		unsigned int startIndexLocation = 0;
		unsigned int baseVertexLocation = 0;
	};
}