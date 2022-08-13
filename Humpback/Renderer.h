// (c) Li Hongcheng
// 2021-10-28

#pragma once

#include <wrl.h>
#include <dxgi1_5.h>
#include <string>
#include <DirectXMath.h>
#include <memory>

#include "Timer.h"
#include "UploadBufferHelper.h"
#include "HMathHelper.h"

using Microsoft::WRL::ComPtr;


namespace Humpback 
{
	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 MVP = HMathHelper::Identity4x4();
	};

	struct SubMesh
	{
	public:

		unsigned int indexCount;
		unsigned int startIndexLocation;
		int baseVertexLocation;
	};

	struct Mesh
	{
	public:

		Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

		// Data about the buffers.
		unsigned int vertexByteStride = 0;
		unsigned int vertexBufferByteSize = 0;
		DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;

		unsigned int indexBufferByteSize = 0;

		std::string Name;

		std::unordered_map<std::string, SubMesh> drawArgs;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
		{
			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
			vbv.StrideInBytes = vertexBufferByteSize;
			vbv.SizeInBytes = vertexBufferByteSize;
			return vbv;
		}

		D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
		{
			D3D12_INDEX_BUFFER_VIEW ibv;
			ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
			ibv.Format = indexFormat;
			ibv.SizeInBytes = indexBufferByteSize;
			return ibv;
		}
	};


	class Renderer
	{
	public:

		Renderer(int width, int height, HWND hwnd);
		Renderer(const Renderer&) = delete;
		Renderer& operator= (const Renderer&) = delete;
		~Renderer();

		static const UINT FrameBufferCount = 2;
		static const UINT TextureWidth = 256;
		static const UINT TextureHeight = 256;
		static const UINT TexturePixelSize = 4; // The number of bytes used to represent a pixel in the texture.

		void Initialize();
		void OnResize();
		void Update();
		void Render();
		void Tick();
		void ShutDown();


		void OnMouseDown(int x, int y);
		void OnMouseUp();
		void OnMouseMove(WPARAM btnState, int x, int y);


	private:

		void _initD3D12();
		void _createCommandObjects();
		void _createSwapChain(IDXGIFactory4*);
		void _createRtvAndDsvDescriptorHeaps();
		void _cleanUp();
		void _waitForPreviousFrame();

		void _createBox();
		void _createDescriptorHeaps();
		void _createConstantBuffers();
		void _createRootSignature();
		void _createShadersAndInputLayout();
		void _createPso();

		void _updateTheViewport();

		D3D12_CPU_DESCRIPTOR_HANDLE _getCurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE _getCurrentDSBufferView();
		ID3D12Resource* _getCurrentBackbuffer();

		std::unique_ptr<Timer>				m_timer = nullptr;

		ComPtr<ID3D12Device>				m_device = nullptr;
		ComPtr<ID3D12CommandQueue>			m_commandQueue = nullptr;
		ComPtr<IDXGISwapChain4>				m_swapChain = nullptr;
		ComPtr<ID3D12DescriptorHeap>		m_rtvHeap = nullptr;
		ComPtr<ID3D12DescriptorHeap>		m_dsvHeap = nullptr;
		ComPtr<ID3D12DescriptorHeap>		m_srvHeap = nullptr;
		ComPtr<ID3D12DescriptorHeap>		m_cbvHeap = nullptr;
		ComPtr<ID3D12Resource>				m_frameBuffers[Renderer::FrameBufferCount];
		ComPtr<ID3D12Resource>				m_depthStencilBuffer = nullptr;
		ComPtr<ID3D12CommandAllocator>		m_commandAllocator = nullptr;
		ComPtr<ID3D12RootSignature>			m_rootSignature = nullptr;
		ComPtr<ID3D12PipelineState>			m_pipelineState = nullptr;
		ComPtr<ID3D12GraphicsCommandList>	m_commandList = nullptr;
		ComPtr<ID3D12Resource>				m_vertexBuffer = nullptr;

		ComPtr<ID3DBlob>					m_vertexShader = nullptr;
		ComPtr<ID3DBlob>					m_pixelShader = nullptr;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

		std::unique_ptr<UploadBuffer<ObjectConstants>> m_constantBuffer = nullptr;
		
		ComPtr<ID3D12Fence>					m_fence = nullptr;
		HANDLE								m_fenceEvent = 0;
		D3D12_VERTEX_BUFFER_VIEW			m_vertexBufferView;
		DXGI_FORMAT							m_frameBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT							m_dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		bool								m_4xMsaaState = false;
		int									m_4xMsaaQuality = 0;

		UINT m_width;
		UINT m_height;
		UINT m_frameIndex;
		HWND m_hwnd;
		float								m_aspectRatio;
		float								m_near = 1.0f;
		float								m_far = 1000.0f;
		std::wstring m_directory;
		UINT m_fenceValue;
		CD3DX12_VIEWPORT					m_viewPort;
		CD3DX12_RECT						m_scissorRect;
		UINT								m_rtvDescriptorSize = 0;

		POINT								m_lastMousePoint;

		DirectX::XMFLOAT4X4							m_viewMatrix;
		DirectX::XMFLOAT4X4							m_projectionMatrix;
		
		std::unique_ptr<Mesh>				m_mesh;
	};
}

