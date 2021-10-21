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

		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;

		UINT m_width;
		UINT m_height;
		HWND m_hwnd;
	};
}

