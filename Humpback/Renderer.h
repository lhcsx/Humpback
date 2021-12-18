// (c) Li Hongcheng
// 2021/10/28

#pragma once

#include<wrl.h>
#include<dxgi1_5.h>
#include<string>
#include<DirectXMath.h>

#include"d3dx12.h"

using Microsoft::WRL::ComPtr;


namespace Humpback {
	class Renderer
	{
	public:

		Renderer(int width, int height, HWND hwnd);
		~Renderer()
		{

		}

		static const UINT BufferCount = 2;
		static const UINT TextureWidth = 256;
		static const UINT TextureHeight = 256;
		static const UINT TexturePixelSize = 4; // The number of bytes used to represent a pixel in the texture.

		void Initialize();
		void Update();
		void Render();
		void Tick();
		void ShutDown();

	private:

		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT2 uv;
		};

		void Clear();
		void LoadPipeline();
		void LoadAssets();
		void WaitForPreviousFrame();
		void PopulateCommandList();
		std::vector<UINT8>	GenerateTextureData();

		ComPtr<ID3D12Device>				m_device;
		ComPtr<ID3D12CommandQueue>			m_commandQueue;
		ComPtr<IDXGISwapChain4>				m_swapChain;
		ComPtr<ID3D12DescriptorHeap>		m_rtvHeap;
		ComPtr<ID3D12DescriptorHeap>		m_srvHeap;
		ComPtr<ID3D12Resource>				m_renderTargets[Renderer::BufferCount];
		ComPtr<ID3D12CommandAllocator>		m_commandAllocator;
		ComPtr<ID3D12RootSignature>			m_rootSignature;
		ComPtr<ID3D12PipelineState>			m_pipelineState;
		ComPtr<ID3D12GraphicsCommandList>	m_commandList;
		ComPtr<ID3D12Resource>				m_vertexBuffer;
		ComPtr<ID3D12Resource>				m_texture;
		
		ComPtr<ID3D12Fence>					m_fence;
		D3D12_VERTEX_BUFFER_VIEW			m_vertexBufferView;
		HANDLE								m_fenceEvent;

		UINT m_width;
		UINT m_height;
		UINT m_frameIndex;
		HWND m_hwnd;
		float m_aspectRatio;
		std::wstring m_directory;
		UINT m_fenceValue;
		CD3DX12_VIEWPORT					m_viewPort;
		CD3DX12_RECT						m_scissorRect;
		UINT								m_rtvDescriptorSize;
	};
}

