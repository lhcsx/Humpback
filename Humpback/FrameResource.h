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
	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 WorldM = HMathHelper::Identity4x4();
	};


	struct PassConstants
	{
		DirectX::XMFLOAT4X4 ViewM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvViewM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ViewProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvViewProjM = HMathHelper::Identity4x4();
		DirectX::XMFLOAT3 CameraPosW = { .0f, .0f, .0f };
		float cbPerObjectPad1 = 0.0f;
		DirectX::XMFLOAT2 RenderTargetSize = { .0f, .0f };
		DirectX::XMFLOAT2 InvRenderTargetSize = { .0f, .0f };
		float NearZ = .0f;
		float FarZ = .0f;
		float TotalTime = 0.0f;
		float DeltaTime = 0.0f;
	};


	class FrameResource
	{
	public:

		FrameResource(ID3D12Device* device, unsigned int passCount, unsigned int objectCount, unsigned int waveVertCount);
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