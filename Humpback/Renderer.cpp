// (c) Li Hongcheng
// 2021-10-28

#include <array>
#include <memory>

#include <wrl.h>
#include <dxgi1_6.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>

#include "HumpbackHelper.h"
#include "Renderer.h"
#include "D3DUtil.h"
#include "Vertex.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::Colors;


namespace Humpback 
{
	Renderer::Renderer(int width, int height, HWND hwnd) :
		m_fenceEvent(0), m_width(width), m_height(height), m_frameIndex(0), m_hwnd(hwnd), 
		m_aspectRatio(m_width / m_height), m_fenceValue(0), m_viewPort(0.f, 0.f, m_width, m_height),
		m_scissorRect(0, 0, m_width, m_height), m_rtvDescriptorSize(0)

	{
	}

	void Renderer::Initialize()
	{
		_createDescriptorHeaps();
		_createConstantBuffers();
		_createRootSignature();
		_createShadersAndInputLayout();
		_createBox();
		_createPso();

		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* commandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		WaitForPreviousFrame();
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

	void Renderer::_createBox()
	{
		// Define vertices.
		std::array<Vertex, 8> vertices =
		{
			Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
			Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
			Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
			Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
			Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
			Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
			Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
			Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
		};

		// Define indices.
		std::array<std::uint16_t, 36> indices =
		{
			// front face
			0, 1, 2,
			0, 2, 3,

			// back face
			4, 6, 5,
			4, 7, 6,

			// left face
			4, 5, 1,
			4, 1, 0,

			// right face
			3, 2, 6,
			3, 6, 7,

			// top face
			1, 5, 6,
			1, 6, 2,

			// bottom face
			4, 0, 3,
			4, 3, 7
		};

		const unsigned int vByteSize = (unsigned int)vertices.size() * sizeof(Vertex);
		const unsigned int iByteSize = (unsigned int)indices.size() * sizeof(uint16_t);

		m_mesh = std::make_unique<Mesh>();
		m_mesh->Name = "MyBox";

		ThrowIfFailed(D3DCreateBlob(vByteSize, &m_mesh->VertexBufferCPU));
		CopyMemory(m_mesh->VertexBufferCPU->GetBufferPointer(), vertices.data(), vByteSize);

		ThrowIfFailed(D3DCreateBlob(iByteSize, &m_mesh->IndexBufferCPU));
		CopyMemory(m_mesh->IndexBufferCPU->GetBufferPointer(), indices.data(), iByteSize);
		
		m_mesh->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			vertices.data(), vByteSize, m_mesh->VertexBufferUploader);

		m_mesh->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			indices.data(), iByteSize, m_mesh->IndexBufferUploader);

		m_mesh->vertexByteStride = sizeof(Vertex);
		m_mesh->vertexBufferByteSize = vByteSize;
		m_mesh->indexFormat = DXGI_FORMAT_R16_UINT;
		m_mesh->indexBufferByteSize = iByteSize;

		SubMesh boxSubMesh;
		boxSubMesh.baseVertexLocation = 0;
		boxSubMesh.startIndexLocation = 0;
		boxSubMesh.indexCount = (unsigned int)indices.size();

		m_mesh->drawArgs["Box"] = boxSubMesh;
	}

	void Renderer::_createDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvDesc;
		cbvDesc.NumDescriptors = 1;
		cbvDesc.NodeMask = 0;
		cbvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvDesc, IID_PPV_ARGS(&m_cbvHeap)));
	}

	void Renderer::_createConstantBuffers()
	{
		m_constantBuffer = std::make_unique<UploadBuffer<ObjectConstants>>();

		unsigned int objCBufferSize = D3DUtil::CalConstantBufferByteSize(sizeof(ObjectConstants));

		D3D12_GPU_VIRTUAL_ADDRESS cbAdress = m_constantBuffer->Resource()->GetGPUVirtualAddress();

		// Offset to the ith object constant buffer in the buffer.
		int objectIdx = 0;
		cbAdress += objCBufferSize * cbAdress;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
		cbDesc.SizeInBytes = objCBufferSize;
		cbDesc.BufferLocation = cbAdress;

		m_device->CreateConstantBufferView(&cbDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void Renderer::_createRootSignature()
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];

		// Create a descriptor table of CBV.
		CD3DX12_DESCRIPTOR_RANGE cbvTable;
		cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, 
			nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
		
		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	void Renderer::_createShadersAndInputLayout()
	{
		UINT compileFlags = 0;

		std::wstring vsPath = L"\\shaders\\Compiled\\SimpleShaderVS.cso";
		std::wstring psPath = L"\\shaders\\Compiled\\SimpleShaderPS.cso";
		vsPath = GetAssetPath(vsPath);
		psPath = GetAssetPath(psPath);

		m_vertexShader = LoadBinary(vsPath);
		m_pixelShader = LoadBinary(psPath);

		m_inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
	}

	void Renderer::_createPso()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psoDesc.InputLayout = { m_inputLayout.data(), (unsigned int)m_inputLayout.size() };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS =
		{
			reinterpret_cast<byte*>(m_vertexShader->GetBufferPointer()),
			m_vertexShader->GetBufferSize()
		};
		psoDesc.PS =
		{
			reinterpret_cast<byte*>(m_pixelShader->GetBufferPointer()),
			m_pixelShader->GetBufferSize()
		};
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = m_rtFormat;
		psoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
		psoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
		psoDesc.DSVFormat = m_dsFormat;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}
}