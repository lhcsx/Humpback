// (c) Li Hongcheng
// 2021/10/28

#include<wrl.h>
#include<dxgi1_6.h>
#include<dxgi1_3.h>
#include<d3dcompiler.h>

#include"d3dx12.h"
#include "Renderer.h"
#include "HumpbackHelper.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

using namespace Microsoft::WRL;

namespace Humpback {
	Renderer::Renderer(int width, int height, HWND hwnd): 
		m_width(width), m_height(height), m_hwnd(hwnd)
	{
	}

	void Renderer::Initialize()
	{
		LoadPipeline();
		LoadAssets();
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

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		// Create descriptor heaps.
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = Renderer::BufferCount;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// Create rander target view.
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE desHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create rtv for each frame buffer.
			for (size_t i = 0; i < Renderer::BufferCount; i++)
			{
				ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
				m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, desHandle);
				desHandle.Offset(1, m_descriptorSize);
			}
		}
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	}

	void Renderer::LoadAssets()
	{
		// Create an empty root signature.
		{
			CD3DX12_ROOT_SIGNATURE_DESC signatureDesc = {};
			signatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			
			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
			ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

			UINT compileFlags = 0;

			std::wstring path = L"\\Shaders\\SimpleShader.hlsl";
			path = GetAssetPath(path);

			ThrowIfFailed(D3DCompileFromFile(path.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
			ThrowIfFailed(D3DCompileFromFile(path.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

			// Input layout.
			D3D12_INPUT_ELEMENT_DESC inputDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// Create the pipeline state.
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputDescs, _countof(inputDescs) };
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = false;
			psoDesc.DepthStencilState.StencilEnable = false;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			
			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		}

		// Create the command list.
	}
}


