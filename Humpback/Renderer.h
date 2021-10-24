#pragma once

#include<d3d12.h>
#include<wrl.h>
#include<dxgi1_5.h>


namespace Humpback {
	class Renderer
	{
	public:

		Renderer(int width, int height, HWND hwnd);
		~Renderer()
		{

		}

		static const UINT BufferCount = 2;

		void Initialize();
		void Update();
		void Render();
		void Tick();
		void ShutDown();

	private:

		void Clear();
		void LoadPipeline();
		void LoadAssets();

		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[Renderer::BufferCount];
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

		UINT m_width;
		UINT m_height;
		UINT m_frameIndex;
		UINT m_descriptorSize;
		HWND m_hwnd;

	};
}

