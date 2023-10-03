// (c) Li Hongcheng
// 2021-10-28

#pragma once

#include <string>
#include <memory>

#include "Camera.h"
#include "Mesh.h"
#include "FrameResource.h"
#include "Timer.h"
#include "UploadBufferHelper.h"
#include "HMathHelper.h"
#include "RenderableObject.h"
#include "Texture.h"
#include "ShadowMap.h"
#include "Light.h"
#include "SSAO.h"


using Microsoft::WRL::ComPtr;


namespace Humpback 
{
	enum class RenderLayer : int
	{
		Opaque = 0,
		Sky,
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

		void OnMouseDown(int x, int y);
		void OnMouseUp();
		void OnMouseMove(WPARAM btnState, int x, int y);
		void OnMouseWheel(short delta);

	private:

		void _initD3D12();
		void _initRendererFeatures();
		void _initTimer();
		void _initCamera();
		void _createCommandObjects();
		void _createSwapChain(IDXGIFactory4*);
		void _createRtvAndDsvDescriptorHeaps();
		void _cleanUp();
		void _waitForPreviousFrame();

		void _createCamera();
		void _createSceneGeometry();
		void _createSimpleGeometry();
		void _loadGeometryFromFile();
		void _createSceneLights();

		void _createRootSignature();
		void _createRootSignatureSSAO();
		void _createShadersAndInputLayout();
		void _createPso();
		void _createFrameResources();
		void _createRenderableObjects();
		void _createMaterialsData();
		
		void _loadTextures();
		void _createDescriptorHeaps();
		void _updateTheViewport();
		CD3DX12_CPU_DESCRIPTOR_HANDLE _getCpuSrv(int idx) const;
		CD3DX12_GPU_DESCRIPTOR_HANDLE _getGpuSrv(int idx) const;
		CD3DX12_CPU_DESCRIPTOR_HANDLE _getDsv(int idx) const;
		CD3DX12_CPU_DESCRIPTOR_HANDLE _getRtv(int idx) const;


		void _render();			// Render per frame.
		void _renderRenderableObjects(ID3D12GraphicsCommandList*, const std::vector<RenderableObject*>&);
		void _renderShadowMap();

		void _update();			// Update per frame.
		void _updateCamera();
		void _updateCBuffers();
		void _updateCBufferPerObject();
		void _updateCBufferPerPass();
		void _updateMatCBuffer();
		void _UpdateShadowCB();
		void _updateInstanceData();
		void _updateShadowMap();
		void _onKeyboardInput();


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
		ComPtr<ID3D12RootSignature>			m_rootSignatureSSAO = nullptr;

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
		bool								m_4xMsaaState = false;
		int									m_4xMsaaQuality = 0;

		unsigned int						m_frameIndex = 0;
		HWND								m_hwnd;
		unsigned int						m_fenceValue = 0;
		CD3DX12_VIEWPORT					m_viewPort;
		CD3DX12_RECT						m_scissorRect;
		unsigned int						m_rtvDescriptorSize = 0;
		POINT								m_lastMousePoint = {0, 0};
		unsigned int						m_cbvSrvUavDescriptorSize = 0;
		unsigned int						m_dsvDescriptorSize = 0;
		unsigned int						m_passCbvOffset = 0;
		
		float								m_theta = 1.5f * DirectX::XM_PI;
		float								m_phi = DirectX::XM_PIDIV4;
		float								m_radius = 5.0f;

		std::unique_ptr<Camera>				m_mainCamera = nullptr;

		std::unordered_map<std::string, std::unique_ptr<Mesh>>		m_meshes;
		std::unordered_map<std::string, std::unique_ptr<Material>>	m_materials;
		std::unordered_map<std::string, std::unique_ptr<Texture>>	m_textures;

		std::vector<std::unique_ptr<RenderableObject>>				m_renderableList;
		std::vector<RenderableObject*>								m_renderLayers[(int)RenderLayer::Count];
		DirectX::BoundingSphere										m_sceneBoundingSphere;

		PassConstants												m_cbufferPerPass;
		PassConstants												m_shadowPassCB;

		unsigned int	m_instanceCount = 0;

		bool			m_enableFrustumCulling = true;

		int				m_skyTexHeapIndex = 0;
		int				m_defaultNormalMapIndex = 0;
		int				m_shadowMapHeapIndex = 0;
		int				m_ssaoHeapIndexStart = 0;
		int				m_ssaoAmbientMapIndex = 0;
		CD3DX12_GPU_DESCRIPTOR_HANDLE	m_nullSrv;

		XMFLOAT4X4		m_lightViewMatrix;
		XMFLOAT4X4		m_lightProjMatrix;
		XMFLOAT4X4		m_ShadowVPTMatrix;

		std::unique_ptr<ShadowMap>	m_shadowMap = nullptr;
		DirectX::XMFLOAT3					m_mainLightPos = { 0.0f, 0.0f, 0.0f };

		std::unique_ptr<DirectionalLight[]> m_directionalLights = nullptr;

		std::unique_ptr<SSAO> m_featureSSAO;
	};
}