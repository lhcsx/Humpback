#include<wrl.h>
#include<dxgi1_6.h>
#include<dxgi1_3.h>

#include "Renderer.h"
#include "HumpbackHelper.h"

#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

namespace Humpback {
	Renderer::Renderer(int width, int height, HWND hwnd): 
		m_width(width), m_height(height), m_hwnd(hwnd)
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
		ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory2(dxgiFlag, IID_PPV_ARGS(&factory)));
		
		// Create device.
		ComPtr<IDXGIAdapter1> pHardwareAdapter;
		GetHardwareAdapter(factory.Get(), pHardwareAdapter.GetAddressOf(), true);
		if (pHardwareAdapter == nullptr)
		{
			return;
		}
		ThrowIfFailed(D3D12CreateDevice(pHardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device)));

		// Create command queue.
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf())));

		// Create swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = Renderer::BufferCount;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Width = m_width;
		swapChainDesc.Height = m_height;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), 
			m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain));

		// Do not support fullscreen transitions.
		ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain.As(&m_swapChain));


	}
}


