// (c) Li Hongcheng
// 2022-08-26


#pragma once


#include <DirectXMath.h>

#include "HMathHelper.h"
#include "UploadBufferHelper.h"
#include "Vertex.h"
#include "Material.h"


namespace Humpback
{
	struct InstanceData;

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 worldM = HMathHelper::Identity4x4();
		unsigned int MaterialIndex;
		unsigned int pat0;
		unsigned int pat1;
		unsigned int pat2;
	};


#define MaxLights 16
	struct LightConstants
	{
		DirectX::XMFLOAT3 strength = { 0.5f, 0.5f, 0.5f };
		float falloffStart = 1.0f;
		DirectX::XMFLOAT3 direction = { 0.0f, -1.0f, 0.0f };
		float falloffEnd = 10.0f;
		DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
		float spotPower = 64.0f;
	};


	struct PassConstants
	{
		DirectX::XMFLOAT4X4 viewM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 invViewM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 projM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 invProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 viewProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 invViewProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 shadowVPT = HMathHelper::Identity4x4();
		DirectX::XMFLOAT3 cameraPosW = { .0f, .0f, .0f };
		float cbPerObjectPad1 = 0.0f;
		DirectX::XMFLOAT2 renderTargetSize = { .0f, .0f };
		DirectX::XMFLOAT2 invRenderTargetSize = { .0f, .0f };
		float nearZ = .0f;
		float farZ = .0f;
		float totalTime = 0.0f;
		float deltaTime = 0.0f;
		
		DirectX::XMFLOAT4 ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		
		LightConstants lights[MaxLights];
	};

	struct SSAOConstants
	{
		DirectX::XMFLOAT4X4 projM;
		DirectX::XMFLOAT4X4 invProjM;
		DirectX::XMFLOAT4X4 projTexM;

		DirectX::XMFLOAT4 offectVectors[14];

		DirectX::XMFLOAT4 weights[3];
		DirectX::XMFLOAT2 PixelSize = { 0.0f, 0.0f };

		float radius = 0.5f;
		float surfaceEpsilon = 0.05f;
		float occlusionFadeStart = 0.2f;
		float occlusionFadeEnd = 2.0f;
	};

	class FrameResource
	{
	public:

		FrameResource(ID3D12Device* device, unsigned int passCount, unsigned int objectCount, 
			unsigned int maxInstanceCount, unsigned int materialCount);
		FrameResource(const FrameResource& rhs) = delete;
		FrameResource& operator=(const FrameResource& rhs) = delete;
		~FrameResource();

		// We cannot reset the cmd allocator until the GPU is done processing the commands in it.
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAlloc;

		std::unique_ptr<UploadBuffer<ObjectConstants>> objCBuffer = nullptr;
		std::unique_ptr<UploadBuffer<PassConstants>> passCBuffer = nullptr;
		std::unique_ptr<UploadBuffer<MaterialConstants>> materialCBuffer = nullptr;
		std::unique_ptr<UploadBuffer<SSAOConstants>> ssaoCBuffer = nullptr;

		//std::unique_ptr<UploadBuffer<InstanceData>> instanceBuffer = nullptr;

		unsigned int fence = 0;
	};
}