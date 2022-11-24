// (c) Li Hongcheng
// 2022-08-26


#include <memory>

#include "FrameResource.h"


namespace Humpback
{

	FrameResource::FrameResource(ID3D12Device* device, unsigned int passCount, unsigned int objectCount, unsigned int materialCount)
	{
		if (device == nullptr)
		{
			return;
		}

		ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(cmdAlloc.GetAddressOf())));

		objCBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
		passCBuffer = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
		materialCBuffer = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
	}

	FrameResource::~FrameResource()
	{
		
	}
}


