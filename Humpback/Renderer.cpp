// (c) Li Hongcheng
// 2021/10/28

#include <wrl.h>
#include <dxgi1_6.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>

#include "d3dx12.h"
#include "Renderer.h"
#include "HumpbackHelper.h"
#include "Box.h"

using namespace Microsoft::WRL;

namespace Humpback {
	Renderer::Renderer(int width, int height, HWND hwnd) :
		m_fenceEvent(0), m_width(width), m_height(height), m_frameIndex(0), m_hwnd(hwnd), 
		m_aspectRatio(m_width / m_height), m_fenceValue(0), m_viewPort(0.f, 0.f, m_width, m_height),
		m_scissorRect(0, 0, m_width, m_height), m_rtvDescriptorSize(0)

	{
	}

	void Renderer::Initialize()
	{
		Prepare();
		LoadPipeline();
		LoadAssets();
	}

	void Renderer::Update()
	{
		// TODO
	}

	void Renderer::Render()
	{
		PopulateCommandList();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		m_swapChain->Present(1, 0);

		WaitForPreviousFrame();
	}

	void Renderer::Tick()
	{
		m_timer->Tick();
		Update();
		Render();
	}

	void Renderer::ShutDown()
	{
		WaitForPreviousFrame();
		Clear();
	}

	void Renderer::OnMouseDown(int x, int y)
	{
		m_lastMousePoint.x = x;
		m_lastMousePoint.y = y;

		SetCapture(m_hwnd);
	}

	void Renderer::OnMouseUp()
	{
		ReleaseCapture();
	}

	void Renderer::OnMouseMove(WPARAM btnState, int x, int y)
	{
		// TODO
	}

	void Renderer::Clear()
	{
		CloseHandle(m_fenceEvent);
		m_timer.reset();
		m_timer.release();
	}

	void Renderer::Prepare()
	{
		m_timer = std::make_unique<Timer>();
		m_timer->Reset();

		// Crate a box.
		m_renderableList = new std::vector<Renderable*>;
		Box* box = new Box();
		m_renderableList->push_back(box);
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
			// rtv
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = Renderer::BufferCount;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			// Describe and create a shader resource view heap for the texture.
			D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
			srvDesc.NumDescriptors = 1;
			srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&m_srvHeap)));


			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// Create rander target view.
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE desHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create rtv for each frame buffer.
			for (size_t i = 0; i < Renderer::BufferCount; i++)
			{
				ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
				m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, desHandle);
				desHandle.Offset(1, m_rtvDescriptorSize);
			}
		}
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	}

	void Renderer::LoadAssets()
	{
		// Create an empty root signature.
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

			// This is the highest version the sample supports. If ChectFeatureSupport succeeds, the HightesVersion returned will not greater than this.			
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
			if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
			{
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

			CD3DX12_ROOT_PARAMETER1 rootParameters[1];
			rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

			D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			samplerDesc.MipLODBias = 0;
			samplerDesc.MaxAnisotropy = 0;
			samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			samplerDesc.MinLOD = 0.0f;
			samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			samplerDesc.ShaderRegister = 0;
			samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			rootSignatureDesc.Init_1_1(_countof(rootParameters), &rootParameters[0], 1, &samplerDesc, 
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
			ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

			UINT compileFlags = 0;

			std::wstring vsPath = L"\\shaders\\Compiled\\SimpleShaderVS.cso";
			std::wstring psPath = L"\\shaders\\Compiled\\SimpleShaderPS.cso";
			vsPath = GetAssetPath(vsPath);
			psPath = GetAssetPath(psPath);

			/*ThrowIfFailed(D3DCompileFromFile(path.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
			ThrowIfFailed(D3DCompileFromFile(path.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));*/

			vertexShader = LoadBinary(vsPath);
			pixelShader = LoadBinary(psPath);


			// Input layout.
			D3D12_INPUT_ELEMENT_DESC inputDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(),
			IID_PPV_ARGS(&m_commandList)));

		// Create the vertex buffer.
		{
			Vertex vertices[] =
			{
				{{0.0f, 0.25f * m_aspectRatio, 0.0f}, {0.5f, 0.0f}},
				{{0.25f, -0.25f * m_aspectRatio, 0.0f}, {1.0f, 1.0f}},
				{{-0.25f, -0.25f * m_aspectRatio, 0.0f}, {0.0f, 1.0f}}
			};

			// The memory size of the vertex array.
			const unsigned int vertexBufferSize = sizeof(vertices);

			D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_vertexBuffer)
			));

			// Copy the triangle data to the vertex buffer.
			UINT8* pVertexDataBegin;
			CD3DX12_RANGE readRange(0, 0);
			ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, vertices, sizeof(vertices));
			m_vertexBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
		}

	
		// Create texture.
		// This resource needs to stay in scope until the command list that references it
		// has finished executing on the GPU.
		ComPtr<ID3D12Resource> textureUploadHeap;
		
		{
			// Describe and create a texture.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			textureDesc.Width = TextureWidth;
			textureDesc.Height = TextureHeight;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = 1;


			auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_texture)));


			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

			// Create the GPU upload buffer.
			auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&uploadBufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap)));


			// Copy data to the intermediate upload heap and then shedule a copy
			// from the upload heap to the Texture2D.
			std::vector<UINT8> texture = GenerateTextureData();

			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = &texture[0];
			textureData.RowPitch = TextureWidth * TexturePixelSize;
			textureData.SlicePitch = textureData.RowPitch * TextureHeight;

			UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
			auto copyDest2ShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			m_commandList->ResourceBarrier(1, &copyDest2ShaderResource);

			// Describ and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
		}

		// Close the command list and execute it to begin the initial GPU setup.
		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* ppCommandList[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
		

		// Create the synchronization objects and wait until assets have been uploaded to the GPU.
		{
			ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceValue = 1;
			m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
			if (m_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}

			// Wait for setup to compelete before continuing.
			WaitForPreviousFrame();
		}


	}

	void Renderer::WaitForPreviousFrame()
	{
		const UINT64 fence = m_fenceValue;
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
		m_fenceValue += 1;

		// Wati untile the previous frame is finished.
		if (m_fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITY);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void Renderer::PopulateCommandList()
	{
		ThrowIfFailed(m_commandAllocator->Reset());

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

		// Set necessary state.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		m_commandList->RSSetViewports(1, &m_viewPort);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);


		// Indicate that the back buffer will be used as a render target.
		auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), 
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &resourceBarrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
		m_commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->DrawInstanced(3, 1, 0, 0);

		auto resourceBarrierRT2Pre = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &resourceBarrierRT2Pre);

		ThrowIfFailed(m_commandList->Close());
	}
	std::vector<UINT8> Renderer::GenerateTextureData()
	{
		const UINT rowPitch = TextureWidth * TexturePixelSize;
		const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
		const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
		const UINT textureSize = rowPitch * TextureHeight;

		std::vector<UINT8> data(textureSize);
		UINT8* pData = &data[0];

		for (UINT n = 0; n < textureSize; n += TexturePixelSize)
		{
			UINT x = n % rowPitch;
			UINT y = n / rowPitch;
			UINT i = x / cellPitch;
			UINT j = y / cellHeight;

			if (i % 2 == j % 2)
			{
				pData[n] = 0x00;        // R
				pData[n + 1] = 0x00;    // G
				pData[n + 2] = 0x00;    // B
				pData[n + 3] = 0xff;    // A
			}
			else
			{
				pData[n] = 0xff;        // R
				pData[n + 1] = 0xff;    // G
				pData[n + 2] = 0xff;    // B
				pData[n + 3] = 0xff;    // A
			}
		}

		return data;
	}
}