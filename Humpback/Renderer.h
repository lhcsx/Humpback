// (c) Li Hongcheng
// 2021-10-28

#pragma once

#include <string>
#include <memory>

#include "Mesh.h"
#include "FrameResource.h"
#include "Timer.h"
#include "UploadBufferHelper.h"
#include "HMathHelper.h"
#include "RenderableObject.h"


using Microsoft::WRL::ComPtr;


namespace Humpback 
{
	enum class RenderLayer : int
	{
		Opaque = 0,
		Count
	};


	class Renderer
	{
	public:

		Renderer(int width, int height, HWND hwnd);
		Renderer(const Renderer&) = delete;
		Renderer& operator= (const Renderer&) = delete;
		~Renderer();

		static const unsigned int FrameBufferCount = 2;

		void Initialize();		// Initialize the rendering engine.
		void OnResize();
		void ShutDown();		// Shut down the engine and clean the resources.

		void Tick();			// Tick the engine.


		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp();
		void OnMouseMove(WPARAM btnState, int x, int y);


	private:

		void _initD3D12();
		void _createCommandObjects();
		void _createSwapChain(IDXGIFactory4*);
		void _createRtvAndDsvDescriptorHeaps();
		void _cleanUp();
		void _waitForPreviousFrame();

		void _createSceneGeometry();
		void _createSimpleGeometry();
		void _loadGeometryFromFile();

		void _createRootSignature();
		void _createShadersAndInputLayout();
		void _createPso();
		void _createFrameResources();
		void _createRenderableObjects();
		void _createMaterialsData();

		void _updateTheViewport();

		void _update();			// Update per frame.
		void _updateCamera();

		void _render();			// Render per frame.
		void _renderRenderableObjects(ID3D12GraphicsCommandList*, const std::vector<RenderableObject*>&);

		void _updateCBuffers();
		void _updateCBufferPerObject();
		void _updateCBufferPerPass();

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
		ComPtr<ID3D12Resource>				m_frameBuffers[Renderer::FrameBufferCount];
		ComPtr<ID3D12Resource>				m_depthStencilBuffer = nullptr;
		ComPtr<ID3D12CommandAllocator>		m_commandAllocator = nullptr;
		ComPtr<ID3D12RootSignature>			m_rootSignature = nullptr;
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_psos;

		ComPtr<ID3D12GraphicsCommandList>	m_commandList = nullptr;
		ComPtr<ID3D12Resource>				m_vertexBuffer = nullptr;

		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

		std::vector<std::unique_ptr<FrameResource>>			m_frameResources;
		FrameResource*						m_curFrameResource = nullptr;
		int									m_curFrameResourceIdx = 0;
		
		ComPtr<ID3D12Fence>					m_fence = nullptr;
		HANDLE								m_fenceEvent = 0;
		DXGI_FORMAT							m_frameBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT							m_dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// Engine config.
		unsigned int						m_width;
		unsigned int						m_height;
		float								m_aspectRatio;
		float								m_near = 1.0f;
		float								m_far = 1000.0f;
		bool								m_4xMsaaState = false;
		int									m_4xMsaaQuality = 0;

		unsigned int						m_frameIndex = 0;
		HWND								m_hwnd;
		unsigned int						m_fenceValue = 0;
		CD3DX12_VIEWPORT					m_viewPort;
		CD3DX12_RECT						m_scissorRect;
		unsigned int						m_rtvDescriptorSize = 0;
		POINT								m_lastMousePoint;
		unsigned int						m_cbvSrvUavDescriptorSize = 0;
		unsigned int						m_passCbvOffset = 0;
		
		float								m_theta = 1.5f * DirectX::XM_PI;
		float								m_phi = DirectX::XM_PIDIV4;
		float								m_radius = 5.0f;

		DirectX::XMFLOAT4X4					m_viewMatrix;
		DirectX::XMFLOAT4X4					m_projectionMatrix;
		
		std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
		std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;

		std::vector<std::unique_ptr<RenderableObject>>		m_renderableList;
		std::vector<RenderableObject*> m_renderLayers[(int)RenderLayer::Count];

		PassConstants						m_cbufferPerPass;

	};
}