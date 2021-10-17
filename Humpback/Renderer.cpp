#include<wrl.h>
#include<dxgi1_6.h>
#include<dxgi1_3.h>

#include "Renderer.h"
#include "HumpbackHelper.h"

#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

namespace Humpback {
	Renderer::Renderer() 
	{
	}

	void Renderer::Initialize()
	{
		LoadPipeline();
	}

	void Renderer::ShutDown()
	{
		Clear();
	}

	void Renderer::Clear()
	{
	}

	void Renderer::LoadPipeline()
	{
		// Initialize DirectX12.
		UINT dxgiFlag = 0;
		ComPtr<IDXGIFactory1> pFactory;
		ThrowIfFailed(CreateDXGIFactory2(dxgiFlag, IID_PPV_ARGS(&pFactory)));
		
		ComPtr<IDXGIAdapter1> pHardwareAdapter;
		GetHardwareAdapter(pFactory.Get(), pHardwareAdapter.GetAddressOf(), true);
		if (pHardwareAdapter == nullptr)
		{
			return;
		}
		ThrowIfFailed(D3D12CreateDevice(pHardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device)));
	}
}


