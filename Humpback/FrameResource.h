// (c) Li Hongcheng
// 2022-08-26


#pragma once


#include <DirectXMath.h>

#include "HMathHelper.h"
#include "UploadBufferHelper.h"
#include "Vertex.h"
#include "Material.h"
#include "Light.h"


namespace Humpback
{
	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 worldM = HMathHelper::Identity4x4();
		unsigned int MaterialIndex;
		unsigned int pat0;
		unsigned int pat1;
		unsigned int pat2;
	};


	struct PassConstants
	{
		DirectX::XMFLOAT4X4 viewM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 invViewM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 projM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 invProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 viewProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 invViewProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT3 cameraPosW = { .0f, .0f, .0f };
		float cbPerObjectPad1 = 0.0f;
		DirectX::XMFLOAT2 renderTargetSize = { .0f, .0f };
		DirectX::XMFLOAT2 invRenderTargetSize = { .0f, .0f };
		float nearZ = .0f;
		float farZ = .0f;
		float totalTime = 0.0f;
		float deltaTime = 0.0f;
		
		DirectX::XMFLOAT4 ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		
		Light lights[MaxLights];
	};


	class FrameResource
	{
	public:

		FrameResource(ID3D12Device* device, unsigned int passCount, unsigned int objectCount, unsigned int materialCount);
		FrameResource(const FrameResource& rhs) = delete;
		FrameResource& operator=(const FrameResource& rhs) = delete;
		~FrameResource();

		// We cannot reset the cmd allocator until the GPU is done processing the commands in it.
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAlloc;

		std::unique_ptr<UploadBuffer<ObjectConstants>> objCBuffer = nullptr;
		std::unique_ptr<UploadBuffer<PassConstants>> passCBuffer = nullptr;
		std::unique_ptr<UploadBuffer<MaterialConstants>> materialCBuffer = nullptr;

		unsigned int fence = 0;
	};
}