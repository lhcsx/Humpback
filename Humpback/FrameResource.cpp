// (c) Li Hongcheng
// 2022-08-26


#include <memory>

#include "FrameResource.h"


namespace Humpback
{

	FrameResource::FrameResource(ID3D12Device* device, unsigned int passCount, unsigned int objectCount)
	{
		if (device == nullptr)
		{
			return;
		}

		objCBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
		passCBuffer = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	}

	FrameResource::~FrameResource()
	{
		
	}
}


