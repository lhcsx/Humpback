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
#include "GeometryGenetator.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::Colors;


namespace Humpback 
{
	extern const int FRAME_RESOURCE_COUNT = 3;

	Renderer::Renderer(int width, int height, HWND hwnd) :
		m_width(width), m_height(height), m_hwnd(hwnd), 
		m_aspectRatio(static_cast<float>(m_width) / m_height), m_viewPort(0.f, 0.f, m_width, m_height),
		m_scissorRect(0, 0, m_width, m_height)

	{
	}

	Renderer::~Renderer()
	{
	}


	void Renderer::Initialize()
	{
		m_timer = std::make_unique<Timer>();
		m_timer->Reset();

		_initD3D12();

		OnResize();

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

		_createRootSignature();
		_createShadersAndInputLayout();
		_createSceneGeometry();
		_createWavesBuffers();
		_createRenderableObjects();
		_createFrameResources();
		_createPso();

		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* commandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		_waitForPreviousFrame();
	}


	void Renderer::_update()
	{
		_updateCamera();

		m_curFrameResourceIdx = (m_curFrameResourceIdx + 1) % FRAME_RESOURCE_COUNT;
		m_curFrameResource = m_frameResources[m_curFrameResourceIdx].get();

		int curFrameFence = m_curFrameResource->fence;
		if (curFrameFence != 0 && m_fence->GetCompletedValue() < curFrameFence)
		{
			// Wait until the GPU has completed the commands in the current frame resource
			// before reusing it.
			ThrowIfFailed(m_fence->SetEventOnCompletion(curFrameFence, 0));
			WaitForSingleObject(m_fenceEvent, INFINITY);
			CloseHandle(m_fenceEvent);
		}

		_updateCBuffers();
		
		_updateWaves();
	}

	void Renderer::_updateCamera()
	{
		// Convert Spherical to Cartesian coordinates.
		float x = m_radius * sinf(m_phi) * cosf(m_theta);
		float z = m_radius * sinf(m_phi) * sinf(m_theta);
		float y = m_radius * cosf(m_phi);


		// Build the view matrix.
		XMVECTOR cameraPos = XMVectorSet(x, y, z, 1.0f);
		XMVECTOR cameraTarget = XMVectorZero();
		XMVECTOR cameraUp = XMVectorSet(.0f, 1.0f, 0.0f, 0.0f);

		XMMATRIX view = XMMatrixLookAtLH(cameraPos, cameraTarget, cameraUp);
		XMStoreFloat4x4(&m_viewMatrix, view);
	}

	void Renderer::_updateCBuffers()
	{
		_updateCBufferPerObject();
		_updateCBufferPerPass();
	}

	void Renderer::_updateCBufferPerObject()
	{
		auto curObjCBuffer = m_curFrameResource->objCBuffer.get();
		for (auto& obj : m_renderableList)
		{
			if (obj->NumFramesDirty > 0)
			{
				XMMATRIX worldM = XMLoadFloat4x4(&(obj->WorldM));

				ObjectConstants objConstants;
				XMStoreFloat4x4(&(objConstants.WorldM), XMMatrixTranspose(worldM));

				curObjCBuffer->CopyData(obj->cbIndex, objConstants);

				obj->NumFramesDirty--;
			}
		}
	}

	void Renderer::_updateCBufferPerPass()
	{
		XMMATRIX viewM = XMLoadFloat4x4(&m_viewMatrix);
		XMMATRIX projM = XMLoadFloat4x4(&m_projectionMatrix);

		XMMATRIX viewProjM = XMMatrixMultiply(viewM, projM);

		XMStoreFloat4x4(&m_cbufferPerPass.ViewM, XMMatrixTranspose(viewM));
		XMStoreFloat4x4(&m_cbufferPerPass.ProjM, XMMatrixTranspose(projM));
		XMStoreFloat4x4(&m_cbufferPerPass.ViewProjM, XMMatrixTranspose(viewProjM));
		m_cbufferPerPass.NearZ = m_near;
		m_cbufferPerPass.FarZ = m_far;
		
		m_curFrameResource->passCBuffer->CopyData(0, m_cbufferPerPass);
	}

	void Renderer::_updateWaves()
	{
		static float tBase = .0f;

		int totaltime = m_timer->TotalTime();
		if (totaltime - tBase >= 0.25f)
		{
			tBase += 0.25f;

			int i = HMathHelper::Rand(4, m_waves->RowCount() - 5);
			int j = HMathHelper::Rand(4, m_waves->ColumnCount() - 5);

			float r = HMathHelper::RandF(0.2f, 0.5f);

			m_waves->Disturb(i, j, r);
		}

		m_waves->Update(m_timer->DeltaTime());

		// Update the wave vertex buffer with the new solution.
		auto currWavesVB = m_curFrameResource->wavesVB.get();
		for (size_t i = 0; i < m_waves->VertexCount(); i++)
		{
			Vertex v;

			v.Position = m_waves->Position(i);
			v.Color = XMFLOAT4(DirectX::Colors::DarkBlue);

			currWavesVB->CopyData(i, v);
		}

		// Set the dynamic VB of the wave renderable object to the current frame VB.
		m_waveObj->mesh->VertexBufferGPU = currWavesVB->Resource();
	}

	void Renderer::_render()
	{
		auto cmdAllocator = m_curFrameResource->cmdAlloc;

		// Reuse the memory associated with command recording.
		// We can only reset the associated command lists have finished execution on the GPU.
		ThrowIfFailed(cmdAllocator->Reset());

		//A command list can be reset after it has been added to the command queue.
		ThrowIfFailed(m_commandList->Reset(cmdAllocator.Get(), m_psos["opaque"].Get()));

		m_commandList->RSSetViewports(1, &m_viewPort);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		auto transP2R = CD3DX12_RESOURCE_BARRIER::Transition(_getCurrentBackbuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &transP2R);

		// Clear depth and color buffers.
		m_commandList->ClearRenderTargetView(_getCurrentBackBufferView(), Colors::DarkGray, 0, nullptr);
		m_commandList->ClearDepthStencilView(_getCurrentDSBufferView(), 
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		auto backbufferView = _getCurrentBackBufferView();
		auto dsView = _getCurrentDSBufferView();
		m_commandList->OMSetRenderTargets(1, &backbufferView, true, &dsView);

		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		
		int passCbvIndex = m_passCbvOffset + m_curFrameResourceIdx;;

		// Bind per-pass constant buffer.
		auto passCB = m_curFrameResource->passCBuffer->Resource();
		m_commandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		_renderRenderableObjects(m_commandList.Get(), m_renderLayers[(int)RenderLayer::Opaque]);

		auto rt2PreBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			_getCurrentBackbuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &rt2PreBarrier);

		ThrowIfFailed(m_commandList->Close());

		// Add the commands to the command queue.
		ID3D12CommandList* commands[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(commands), commands);

		ThrowIfFailed(m_swapChain->Present(0, 0));
		m_frameIndex = (m_frameIndex + 1) % FrameBufferCount;

		m_curFrameResource->fence = (++m_fenceValue);

		m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
	}


	void Renderer::_renderRenderableObjects(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderableObject*>& objList)
	{
		if (cmdList == nullptr)
		{
			ThrowInvalidParameterException();
		}

		auto objCB = m_curFrameResource->objCBuffer->Resource();
		auto objCBByteSize = D3DUtil::CalConstantBufferByteSize(sizeof(ObjectConstants));

		for (size_t i = 0; i < objList.size(); i++)
		{
			auto obj = objList[i];
			if (obj == nullptr)
			{
				continue;
			}

			auto vbv = obj->mesh->VertexBufferView();
			cmdList->IASetVertexBuffers(0, 1, &vbv);

			auto ibv = obj->mesh->IndexBufferView();
			cmdList->IASetIndexBuffer(&ibv);

			cmdList->IASetPrimitiveTopology(obj->primitiveTopology);

			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objCB->GetGPUVirtualAddress();
			objCBAddress += obj->cbIndex * objCBByteSize;

			cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);


			cmdList->DrawIndexedInstanced(obj->indexCount, 1, obj->startIndexLocation, obj->baseVertexLocation, 0);
		}
	}

	void Renderer::OnResize()
	{
		_waitForPreviousFrame();

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

		// Release previous resources.
		for (size_t i = 0; i < FrameBufferCount; i++)
		{
			m_frameBuffers[i].Reset();
		}
		m_depthStencilBuffer.Reset();

		// Reset the swap chain.
		ThrowIfFailed(m_swapChain->ResizeBuffers(FrameBufferCount, m_width, m_height, m_frameBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		m_frameIndex = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (size_t i = 0; i < FrameBufferCount; i++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_frameBuffers[i])));
			m_device->CreateRenderTargetView(m_frameBuffers[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
		}

		// Recreate depth/stencil buffer and view.
		D3D12_RESOURCE_DESC dsDesc;
		dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		dsDesc.Alignment = 0;
		dsDesc.Width = m_width;
		dsDesc.Height = m_height;
		dsDesc.MipLevels = 1;
		dsDesc.DepthOrArraySize = 1;
		dsDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		dsDesc.SampleDesc.Count = 1;
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE fbClear;
		fbClear.Format = m_dsFormat;
		fbClear.DepthStencil.Depth = 1.0f;
		fbClear.DepthStencil.Stencil = .0f;
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
			&dsDesc, D3D12_RESOURCE_STATE_COMMON, &fbClear, IID_PPV_ARGS(&m_depthStencilBuffer)));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = m_dsFormat;
		dsvDesc.Texture2D.MipSlice = 0;
		m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, 
			m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

		auto bar = CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		m_commandList->ResourceBarrier(1, &bar);

		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

		_waitForPreviousFrame();

		_updateTheViewport();

		XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * HMathHelper::PI, m_aspectRatio, m_near, m_far);
		XMStoreFloat4x4(&m_projectionMatrix, p);
	}



	void Renderer::Tick()
	{
		m_timer->Tick();
		_update();
		_render();
	}

	void Renderer::ShutDown()
	{
		_waitForPreviousFrame();
		_cleanUp();
	}

	void Renderer::OnMouseDown(WPARAM btnState, int x, int y)
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
		if ((btnState & MK_LBUTTON) != 0)
		{
			float dx = XMConvertToRadians(0.25 * static_cast<float>(x - m_lastMousePoint.x));
			float dy = XMConvertToRadians(0.25 * static_cast<float>(y - m_lastMousePoint.y));

			m_theta += dx;
			m_phi += dy;

			m_phi = HMathHelper::Clamp(m_phi, 0.1f, HMathHelper::PI - 0.1f);
		}
		else if((btnState & MK_RBUTTON) != 0)
		{
			float dx = 0.005f * static_cast<float>(x - m_lastMousePoint.x);
			float dy = 0.005f * static_cast<float>(y - m_lastMousePoint.y);

			m_radius += dx - dy;

			m_radius = HMathHelper::Clamp(m_radius, 3.0f, 15.0f);
		}

		m_lastMousePoint.x = x;
		m_lastMousePoint.y = y;
	}

	void Renderer::_cleanUp()
	{
		m_timer.reset();
		m_timer.release();
	}

	void Renderer::_waitForPreviousFrame()
	{
		m_fenceValue++;

		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

		// Wati untile the previous frame is finished.
		if (m_fence->GetCompletedValue() < m_fenceValue)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITY);
		}
	}

	void Renderer::_createSceneGeometry()
	{
		m_waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

		GeometryGenerator geoGen;
		GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

		std::vector<Vertex> vertices(grid.vertices.size());
		for (size_t i = 0; i < grid.vertices.size(); i++)
		{
			auto& p = grid.vertices[i].Position;
			vertices[i].Position = p;
			vertices[i].Position.y = geoGen.GetHillsHeight(p.x, p.z);

			// Color the vertex based on its height.
			if (vertices[i].Position.y < -10.0f)
			{
				// Sandy beach color.
				vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
			}
			else if (vertices[i].Position.y < 5.0f)
			{
				// Light yellow-green.
				vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
			}
			else if (vertices[i].Position.y < 12.0f)
			{
				// Dark yellow-green.
				vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
			}
			else if (vertices[i].Position.y < 20.0f)
			{
				// Dark brown.
				vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
			}
			else
			{
				// White snow.
				vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		
		std::vector<std::uint16_t> indices = grid.GetIndices16();
		
		const unsigned int vbByteSize = vertices.size() * sizeof(Vertex);
		const unsigned int ibByteSize = indices.size() * sizeof(std::uint16_t);

		auto mesh = std::make_unique<Mesh>();
		mesh->Name = "land";
		
		ThrowIfFailed(D3DCreateBlob(vbByteSize, &mesh->VertexBufferCPU));
		CopyMemory(mesh->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &mesh->IndexBufferCPU));
		CopyMemory(mesh->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		mesh->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			vertices.data(), vbByteSize, mesh->VertexBufferUploader);

		mesh->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			indices.data(), ibByteSize, mesh->IndexBufferUploader);

		mesh->vertexByteStride = sizeof(Vertex);
		mesh->vertexBufferByteSize = vbByteSize;
		mesh->indexFormat = DXGI_FORMAT_R16_UINT;
		mesh->indexBufferByteSize = ibByteSize;

		SubMesh subMesh;
		subMesh.baseVertexLocation = 0;
		subMesh.indexCount = indices.size();
		subMesh.startIndexLocation = 0;

		mesh->drawArgs["grid"] = subMesh;

		m_meshes["landGeo"] = std::move(mesh);
	}

	void Renderer::_createWavesBuffers()
	{
		std::vector<std::uint16_t> indices(3 * m_waves->TriangleCount());

		int m = m_waves->RowCount();
		int n = m_waves->ColumnCount();
		int k = 0;
		for (size_t i = 0; i < m - 1; i++)
		{
			for (size_t j = 0; j < n - 1; j++)
			{
				indices[k] = i * n + j;
				indices[k + 1] = i * n + j + 1;
				indices[k + 2] = (i + 1) * n + j;

				indices[k + 3] = (i + 1) * n + j;
				indices[k + 4] = i * n + j + 1;
				indices[k + 5] = (i + 1) * n + j + 1;

				k += 6; // next quad
			}
		}
		
		unsigned int vbByteSize = m_waves->VertexCount() * sizeof(Vertex);
		unsigned int ibByteSize = (unsigned int)indices.size() * sizeof(std::uint16_t);
		
		auto mesh = std::make_unique<Mesh>();
		mesh->Name = "wateGeo";

		// set dynamically.
		mesh->VertexBufferCPU = nullptr;
		mesh->VertexBufferGPU = nullptr;

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &mesh->IndexBufferCPU));
		CopyMemory(mesh->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		mesh->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			indices.data(), ibByteSize, mesh->IndexBufferUploader);

		mesh->vertexByteStride = sizeof(Vertex);
		mesh->vertexBufferByteSize = vbByteSize;
		mesh->indexFormat = DXGI_FORMAT_R16_UINT;
		mesh->indexBufferByteSize = ibByteSize;

		SubMesh sm;
		sm.indexCount = indices.size();
		sm.baseVertexLocation = 0;
		sm.startIndexLocation = 0;
		
		mesh->drawArgs["grid"] = sm;
		m_meshes["waterGeo"] = std::move(mesh);
	}

	void Renderer::_initD3D12()
	{
#if defined(DEBUG) || defined(_DEBUG) 
		// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debugController;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
#endif

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

		// Do not support fullscreen transitions.
		ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));


		_createRtvAndDsvDescriptorHeaps();
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		
		_createCommandObjects();
		_createSwapChain(factory.Get());
	}

	void Renderer::_createCommandObjects()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

		// It needs to be closed before resetting it.
		m_commandList->Close();
	}

	void Renderer::_createSwapChain(IDXGIFactory4* factory)
	{
		if (factory == nullptr)
		{
			return;
		}

		// Create swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = Renderer::FrameBufferCount;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Width = m_width;
		swapChainDesc.Height = m_height;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		ComPtr<IDXGISwapChain1> pSwapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(m_commandQueue.Get(),
			m_hwnd, &swapChainDesc, nullptr, nullptr, &pSwapChain));
		ThrowIfFailed(pSwapChain.As(&m_swapChain));

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void Renderer::_createRtvAndDsvDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDesc.NumDescriptors = FrameBufferCount;
		rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvDesc.NodeMask = 0;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
		dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvDesc.NumDescriptors = FrameBufferCount;
		dsvDesc.NodeMask = 0;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_dsvHeap)));
	}

	void Renderer::_createRootSignature()
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[2];

		// Create root CBVs
		slotRootParameter[0].InitAsConstantBufferView(0);
		slotRootParameter[1].InitAsConstantBufferView(1);


		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, 
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
		// Load shaders pre-compiled.
		//std::wstring vsPath = L"\\shaders\\Compiled\\SimpleShaderVS.cso";
		//std::wstring psPath = L"\\shaders\\Compiled\\SimpleShaderPS.cso";
		//vsPath = GetAssetPath(vsPath);
		//psPath = GetAssetPath(psPath);

		//m_vertexShader = LoadBinary(vsPath);
		//m_pixelShader = LoadBinary(psPath);

		
		// Compile shaders in Runtime for easily debugging.
		std::wstring shaderPath = L"\\shaders\\SimpleShader.hlsl";
		std::wstring shaderFullPath = GetAssetPath(shaderPath);

		m_shaders["standardVS"] = D3DUtil::CompileShader(shaderFullPath, nullptr, "VSMain", "vs_5_0");
		m_shaders["opaquePS"] = D3DUtil::CompileShader(shaderFullPath, nullptr, "PSMain", "ps_5_0");

		m_inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
	}

	void Renderer::_createPso()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

		//
		// PSO for opaque objects.
		//
		ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		opaquePsoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
		opaquePsoDesc.pRootSignature = m_rootSignature.Get();
		opaquePsoDesc.VS =
		{
			reinterpret_cast<BYTE*>(m_shaders["standardVS"]->GetBufferPointer()),
			m_shaders["standardVS"]->GetBufferSize()
		};
		opaquePsoDesc.PS =
		{
			reinterpret_cast<BYTE*>(m_shaders["opaquePS"]->GetBufferPointer()),
			m_shaders["opaquePS"]->GetBufferSize()
		};
		opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		opaquePsoDesc.SampleMask = UINT_MAX;
		opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		opaquePsoDesc.NumRenderTargets = 1;
		opaquePsoDesc.RTVFormats[0] = m_frameBufferFormat;
		opaquePsoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
		opaquePsoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
		opaquePsoDesc.DSVFormat = m_dsFormat;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_psos["opaque"])));


		//
		// PSO for opaque wireframe objects.
		//

		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
		opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_psos["opaque_wireframe"])));
	}

	void Renderer::_createRenderableObjects()
	{
		auto wavesGO = std::make_unique<RenderableObject>();
		wavesGO->mesh = m_meshes["waterGeo"].get();
		auto wavesSubMesh = wavesGO->mesh->drawArgs["grid"];
		wavesGO->WorldM = HMathHelper::Identity4x4();
		wavesGO->cbIndex = 0;
		wavesGO->primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wavesGO->indexCount = wavesSubMesh.indexCount;
		wavesGO->startIndexLocation = wavesSubMesh.startIndexLocation;
		wavesGO->baseVertexLocation = wavesSubMesh.baseVertexLocation;

		m_waveObj = wavesGO.get();

		m_renderLayers[(int)RenderLayer::Opaque].push_back(wavesGO.get());

		
		auto gridGO = std::make_unique<RenderableObject>();
		gridGO->WorldM = HMathHelper::Identity4x4();
		gridGO->cbIndex = 1;
		gridGO->mesh = m_meshes["landGeo"].get();
		gridGO->primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		gridGO->indexCount = wavesSubMesh.indexCount;
		gridGO->startIndexLocation = wavesSubMesh.startIndexLocation;
		gridGO->baseVertexLocation = wavesSubMesh.baseVertexLocation;

		m_renderLayers[(int)RenderLayer::Opaque].push_back(gridGO.get());

		m_renderableList.push_back(std::move(wavesGO));
		m_renderableList.push_back(std::move(gridGO));
	}

	void Renderer::_createFrameResources()
	{
		for (size_t i = 0; i < FRAME_RESOURCE_COUNT; i++)
		{
			m_frameResources.push_back(
				std::make_unique<FrameResource>(m_device.Get(), 1, m_renderableList.size(), m_waves->VertexCount()));
		}
	}

	void Renderer::_updateTheViewport()
	{
		m_viewPort.TopLeftX = 0;
		m_viewPort.TopLeftY = 0;
		m_viewPort.Width = static_cast<float>(m_width);
		m_viewPort.Height = static_cast<float>(m_height);
		m_viewPort.MinDepth = .0f;
		m_viewPort.MaxDepth = 1.f;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Renderer::_getCurrentBackBufferView()
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Renderer::_getCurrentDSBufferView()
	{
		return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	ID3D12Resource* Renderer::_getCurrentBackbuffer()
	{
		return m_frameBuffers[m_frameIndex].Get();
	}
}