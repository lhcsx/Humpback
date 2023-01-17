// (c) Li Hongcheng
// 2022-08-26


#include <memory>

#include "FrameResource.h"
#include "RenderableObject.h"


namespace Humpback
{

	FrameResource::FrameResource(ID3D12Device* device, unsigned int passCount, unsigned int objectCount, 
		unsigned int maxInstanceCount, unsigned int materialCount)
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
		materialCBuffer = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, false);
		instanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, maxInstanceCount, false);
	}

	FrameResource::~FrameResource()
	{
		
	}
}


