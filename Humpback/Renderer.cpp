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
	class Material;

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
		_initTimer();

		_initD3D12();

		OnResize();

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

		_createRootSignature();
		_createShadersAndInputLayout();
		_createSceneGeometry();
		_createMaterialsData();
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
	}

	void Renderer::_updateCamera()
	{
		// Convert Spherical to Cartesian coordinates.
		m_cameraPos.x = m_radius * sinf(m_phi) * cosf(m_theta);
		m_cameraPos.z = m_radius * sinf(m_phi) * sinf(m_theta);
		m_cameraPos.y = m_radius * cosf(m_phi);

		// Build the view matrix.
		XMVECTOR cameraPos = XMVectorSet(m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, 1.0f);
		XMVECTOR cameraTarget = XMVectorZero();
		XMVECTOR cameraUp = XMVectorSet(.0f, 1.0f, 0.0f, 0.0f);


		XMMATRIX view = XMMatrixLookAtLH(cameraPos, cameraTarget, cameraUp);
		XMStoreFloat4x4(&m_viewMatrix, view);
	}

	void Renderer::_updateCBuffers()
	{
		_updateCBufferPerObject();
		_updateMatCBuffer();
		_updateCBufferPerPass();
	}

	void Renderer::_updateCBufferPerObject()
	{
		auto curObjCBuffer = m_curFrameResource->objCBuffer.get();
		for (auto& obj : m_renderableList)
		{
			if (obj->NumFramesDirty > 0)
			{
				XMMATRIX worldM = XMLoadFloat4x4(&(obj->worldM));

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

		XMStoreFloat4x4(&m_cbufferPerPass.viewM, XMMatrixTranspose(viewM));
		XMStoreFloat4x4(&m_cbufferPerPass.projM, XMMatrixTranspose(projM));
		XMStoreFloat4x4(&m_cbufferPerPass.viewProjM, XMMatrixTranspose(viewProjM));
		m_cbufferPerPass.nearZ = m_near;
		m_cbufferPerPass.farZ = m_far;
		m_cbufferPerPass.cameraPosW = m_cameraPos;
		m_cbufferPerPass.ambient = XMFLOAT4(0.2f, 0.2f, 0.3f, 1.0f);
		m_cbufferPerPass.lights[0].direction = { 0.57735f, -0.57735f, 0.57735f };
		m_cbufferPerPass.lights[0].strength = { 0.6f, 0.6f, 0.6f };
		m_cbufferPerPass.lights[1].direction = { -0.57735f, -0.57735f, 0.57735f };
		m_cbufferPerPass.lights[1].strength = { 0.3f, 0.3f, 0.3f };
		m_cbufferPerPass.lights[2].direction = { 0.0f, -0.707f, -0.707f };
		m_cbufferPerPass.lights[2].strength = { 0.15f, 0.15f, 0.15f };
		
		m_curFrameResource->passCBuffer->CopyData(0, m_cbufferPerPass);
	}

	void Renderer::_updateMatCBuffer()
	{
		auto curMatCB = m_curFrameResource->materialCBuffer.get();
		for (auto& m : m_materials)
		{
			Material* pMat = m.second.get();
			if (pMat->numFramesDirty > 0)
			{
				
				MaterialConstants matConstants;
				matConstants.diffuseAlbedo = pMat->diffuseAlbedo;
				matConstants.fresnelR0 = pMat->fresnelR0;
				matConstants.roughness = pMat->roughness;
				
				XMMATRIX matrixMatTrans = XMLoadFloat4x4(&pMat->matTransform);
				XMStoreFloat4x4(&matConstants.matTransform, XMMatrixTranspose(matrixMatTrans));

				curMatCB->CopyData(pMat->matCBIdx, matConstants);

				--pMat->numFramesDirty;
			}
		}
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
		
		// Bind per-pass constant buffer.
		auto passCB = m_curFrameResource->passCBuffer->Resource();
		m_commandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

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

		auto matCB = m_curFrameResource->materialCBuffer->Resource();
		auto matCBByteSize = D3DUtil::CalConstantBufferByteSize(sizeof(MaterialConstants));

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

			D3D12_GPU_VIRTUAL_ADDRESS matCBAdress = matCB->GetGPUVirtualAddress();
			matCBAdress += obj->material->matCBIdx * matCBByteSize;


			cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
			cmdList->SetGraphicsRootConstantBufferView(1, matCBAdress);


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

	void Renderer::_initTimer()
	{
		m_timer = std::make_unique<Timer>();
		m_timer->Reset();
	}

	void Renderer::_createSceneGeometry()
	{
		_createSimpleGeometry();
		_loadGeometryFromFile();
	}

	void Renderer::_createSimpleGeometry()
	{
		GeometryGenerator geoGen;
		GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
		GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
		GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
		GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

		//
		// We are concatenating all the geometry into one big vertex/index buffer.  So
		// define the regions in the buffer each submesh covers.
		//

		// Cache the vertex offsets to each object in the concatenated vertex buffer.
		UINT boxVertexOffset = 0;
		UINT gridVertexOffset = (UINT)box.vertices.size();
		UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.vertices.size();
		UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.vertices.size();

		// Cache the starting index for each object in the concatenated index buffer.
		UINT boxIndexOffset = 0;
		UINT gridIndexOffset = (UINT)box.indices32.size();
		UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.indices32.size();
		UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.indices32.size();

		SubMesh boxSubmesh;
		boxSubmesh.indexCount = (UINT)box.indices32.size();
		boxSubmesh.startIndexLocation = boxIndexOffset;
		boxSubmesh.baseVertexLocation = boxVertexOffset;

		SubMesh gridSubmesh;
		gridSubmesh.indexCount = (UINT)grid.indices32.size();
		gridSubmesh.startIndexLocation = gridIndexOffset;
		gridSubmesh.baseVertexLocation = gridVertexOffset;

		SubMesh sphereSubmesh;
		sphereSubmesh.indexCount = (UINT)sphere.indices32.size();
		sphereSubmesh.startIndexLocation = sphereIndexOffset;
		sphereSubmesh.baseVertexLocation = sphereVertexOffset;

		SubMesh cylinderSubmesh;
		cylinderSubmesh.indexCount = (UINT)cylinder.indices32.size();
		cylinderSubmesh.startIndexLocation = cylinderIndexOffset;
		cylinderSubmesh.baseVertexLocation = cylinderVertexOffset;

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		auto totalVertexCount =
			box.vertices.size() +
			grid.vertices.size() +
			sphere.vertices.size() +
			cylinder.vertices.size();

		std::vector<Vertex> vertices(totalVertexCount);

		UINT k = 0;
		for (size_t i = 0; i < box.vertices.size(); ++i, ++k)
		{
			vertices[k].position = box.vertices[i].Position;
			vertices[k].normal = box.vertices[i].Normal;
		}

		for (size_t i = 0; i < grid.vertices.size(); ++i, ++k)
		{
			vertices[k].position = grid.vertices[i].Position;
			vertices[k].normal = grid.vertices[i].Normal;
		}

		for (size_t i = 0; i < sphere.vertices.size(); ++i, ++k)
		{
			vertices[k].position = sphere.vertices[i].Position;
			vertices[k].normal = sphere.vertices[i].Normal;
		}

		for (size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
		{
			vertices[k].position = cylinder.vertices[i].Position;
			vertices[k].normal = cylinder.vertices[i].Normal;
		}

		std::vector<std::uint16_t> indices;
		indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
		indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
		indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
		indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<Mesh>();
		geo->Name = "shapeGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCPU));
		CopyMemory(geo->vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCPU));
		CopyMemory(geo->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->vertexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(),
			m_commandList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);

		geo->indexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(),
			m_commandList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

		geo->vertexByteStride = sizeof(Vertex);
		geo->vertexBufferByteSize = vbByteSize;
		geo->indexFormat = DXGI_FORMAT_R16_UINT;
		geo->indexBufferByteSize = ibByteSize;

		geo->drawArgs["box"] = boxSubmesh;
		geo->drawArgs["grid"] = gridSubmesh;
		geo->drawArgs["sphere"] = sphereSubmesh;
		geo->drawArgs["cylinder"] = cylinderSubmesh;

		m_meshes[geo->Name] = std::move(geo);
	}

	void Renderer::_loadGeometryFromFile()
	{
		std::ifstream fin("Models/skull.txt");

		if (!fin)
		{
			MessageBox(0, L"Models/skull.txt not found", 0, 0);
			return;
		}

		unsigned int vertCount = 0;
		unsigned int primCount = 0;
		std::string ignore;

		fin >> ignore >> vertCount;
		fin >> ignore >> primCount;
		fin >> ignore >> ignore >> ignore >> ignore;

		std::vector<Vertex> vertices(vertCount);
		for (size_t i = 0; i < vertCount; i++)
		{
			fin >> vertices[i].position.x >> vertices[i].position.y >> vertices[i].position.z;
			fin >> vertices[i].normal.x >> vertices[i].normal.y >> vertices[i].normal.z;
		}

		fin >> ignore >> ignore >> ignore;

		std::vector<std::int32_t> indices(primCount * 3);
		for (size_t i = 0; i < primCount; i++)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		unsigned int vbByteSize = vertices.size() * sizeof(Vertex);
		unsigned int ibByteSize = indices.size() * sizeof(std::uint32_t);

		auto skullMesh = std::make_unique<Mesh>();
		skullMesh->Name = "skullGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &(skullMesh->vertexBufferCPU)));
		CopyMemory(skullMesh->vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &(skullMesh->indexBufferCPU)));
		CopyMemory(skullMesh->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		skullMesh->vertexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			vertices.data(), vbByteSize, skullMesh->vertexBufferUploader);

		skullMesh->indexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			indices.data(), ibByteSize, skullMesh->indexBufferUploader);

		skullMesh->vertexByteStride = sizeof(Vertex);
		skullMesh->vertexBufferByteSize = vbByteSize;
		skullMesh->indexFormat = DXGI_FORMAT_R32_UINT;
		skullMesh->indexBufferByteSize = ibByteSize;

		SubMesh skullSM;
		skullSM.indexCount = indices.size();
		skullSM.baseVertexLocation = 0;
		skullSM.startIndexLocation = 0;

		skullMesh->drawArgs["skull"] = skullSM;

		m_meshes[skullMesh->Name] = std::move(skullMesh);
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
		CD3DX12_ROOT_PARAMETER slotRootParameter[3];

		// Create root CBVs
		slotRootParameter[0].InitAsConstantBufferView(0);
		slotRootParameter[1].InitAsConstantBufferView(1);
		slotRootParameter[2].InitAsConstantBufferView(2);


		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, 
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
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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
		auto boxGO = std::make_unique<RenderableObject>();
		XMStoreFloat4x4(&boxGO->worldM, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
		XMStoreFloat4x4(&boxGO->texTrans, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		boxGO->cbIndex = 0;
		boxGO->material = m_materials["mat_stone"].get();
		boxGO->mesh = m_meshes["shapeGeo"].get();
		boxGO->indexCount = boxGO->mesh->drawArgs["box"].indexCount;
		boxGO->startIndexLocation = boxGO->mesh->drawArgs["box"].startIndexLocation;
		boxGO->baseVertexLocation = boxGO->mesh->drawArgs["box"].baseVertexLocation;
		m_renderableList.push_back(std::move(boxGO));

		auto gridRitem = std::make_unique<RenderableObject>();
		gridRitem->worldM = HMathHelper::Identity4x4();
		XMStoreFloat4x4(&gridRitem->texTrans, XMMatrixScaling(8.0f, 8.0f, 1.0f));
		gridRitem->cbIndex = 1;
		gridRitem->material = m_materials["mat_tile"].get();
		gridRitem->mesh = m_meshes["shapeGeo"].get();
		gridRitem->indexCount = gridRitem->mesh->drawArgs["grid"].indexCount;
		gridRitem->startIndexLocation = gridRitem->mesh->drawArgs["grid"].startIndexLocation;
		gridRitem->baseVertexLocation = gridRitem->mesh->drawArgs["grid"].baseVertexLocation;
		m_renderableList.push_back(std::move(gridRitem));

		auto skullRitem = std::make_unique<RenderableObject>();
		XMStoreFloat4x4(&skullRitem->worldM, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
		skullRitem->texTrans = HMathHelper::Identity4x4();
		skullRitem->cbIndex = 2;
		skullRitem->material = m_materials["mat_skull"].get();
		skullRitem->mesh = m_meshes["skullGeo"].get();
		skullRitem->indexCount = skullRitem->mesh->drawArgs["skull"].indexCount;
		skullRitem->startIndexLocation = skullRitem->mesh->drawArgs["skull"].startIndexLocation;
		skullRitem->baseVertexLocation = skullRitem->mesh->drawArgs["skull"].baseVertexLocation;
		m_renderableList.push_back(std::move(skullRitem));

		XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
		UINT objCBIndex = 3;
		for (int i = 0; i < 5; ++i)
		{
			auto leftCylRitem = std::make_unique<RenderableObject>();
			auto rightCylRitem = std::make_unique<RenderableObject>();
			auto leftSphereRitem = std::make_unique<RenderableObject>();
			auto rightSphereRitem = std::make_unique<RenderableObject>();

			XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
			XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

			XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
			XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

			XMStoreFloat4x4(&leftCylRitem->worldM, rightCylWorld);
			XMStoreFloat4x4(&leftCylRitem->texTrans, brickTexTransform);
			leftCylRitem->cbIndex = objCBIndex++;
			leftCylRitem->material = m_materials["mat_bricks"].get();
			leftCylRitem->mesh = m_meshes["shapeGeo"].get();
			leftCylRitem->indexCount = leftCylRitem->mesh->drawArgs["cylinder"].indexCount;
			leftCylRitem->startIndexLocation = leftCylRitem->mesh->drawArgs["cylinder"].startIndexLocation;
			leftCylRitem->baseVertexLocation = leftCylRitem->mesh->drawArgs["cylinder"].baseVertexLocation;

			XMStoreFloat4x4(&rightCylRitem->worldM, leftCylWorld);
			XMStoreFloat4x4(&rightCylRitem->texTrans, brickTexTransform);
			rightCylRitem->cbIndex = objCBIndex++;
			rightCylRitem->material = m_materials["mat_bricks"].get();
			rightCylRitem->mesh = m_meshes["shapeGeo"].get();
			rightCylRitem->indexCount = rightCylRitem->mesh->drawArgs["cylinder"].indexCount;
			rightCylRitem->startIndexLocation = rightCylRitem->mesh->drawArgs["cylinder"].startIndexLocation;
			rightCylRitem->baseVertexLocation = rightCylRitem->mesh->drawArgs["cylinder"].baseVertexLocation;

			XMStoreFloat4x4(&leftSphereRitem->worldM, leftSphereWorld);
			leftSphereRitem->texTrans = HMathHelper::Identity4x4();
			leftSphereRitem->cbIndex = objCBIndex++;
			leftSphereRitem->material = m_materials["mat_stone"].get();
			leftSphereRitem->mesh = m_meshes["shapeGeo"].get();
			leftSphereRitem->indexCount = leftSphereRitem->mesh->drawArgs["sphere"].indexCount;
			leftSphereRitem->startIndexLocation = leftSphereRitem->mesh->drawArgs["sphere"].startIndexLocation;
			leftSphereRitem->baseVertexLocation = leftSphereRitem->mesh->drawArgs["sphere"].baseVertexLocation;

			XMStoreFloat4x4(&rightSphereRitem->worldM, rightSphereWorld);
			rightSphereRitem->texTrans = HMathHelper::Identity4x4();
			rightSphereRitem->cbIndex = objCBIndex++;
			rightSphereRitem->material = m_materials["mat_stone"].get();
			rightSphereRitem->mesh = m_meshes["shapeGeo"].get();
			rightSphereRitem->indexCount = rightSphereRitem->mesh->drawArgs["sphere"].indexCount;
			rightSphereRitem->startIndexLocation = rightSphereRitem->mesh->drawArgs["sphere"].startIndexLocation;
			rightSphereRitem->baseVertexLocation = rightSphereRitem->mesh->drawArgs["sphere"].baseVertexLocation;

			m_renderableList.push_back(std::move(leftCylRitem));
			m_renderableList.push_back(std::move(rightCylRitem));
			m_renderableList.push_back(std::move(leftSphereRitem));
			m_renderableList.push_back(std::move(rightSphereRitem));
		}

		for (auto& e : m_renderableList)
		{
			m_renderLayers[(int)RenderLayer::Opaque].push_back(e.get());
		}
	}	

	void Renderer::_createMaterialsData()
	{
		auto bricksMat = std::make_unique<Material>();
		bricksMat->name = "mat_bricks";
		bricksMat->matCBIdx = 0;
		bricksMat->diffuseSrvHeapIndex = 0;
		bricksMat->diffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
		bricksMat->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
		bricksMat->roughness = 0.1f;

		auto stoneMat = std::make_unique<Material>();
		stoneMat->name = "mat_stone";
		stoneMat->matCBIdx = 1;
		stoneMat->diffuseSrvHeapIndex = 1;
		stoneMat->diffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
		stoneMat->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
		stoneMat->roughness = 0.3f;

		auto tileMat = std::make_unique<Material>();
		tileMat->name = "mat_tile";
		tileMat->matCBIdx = 2;
		tileMat->diffuseSrvHeapIndex = 2;
		tileMat->diffuseAlbedo = XMFLOAT4(Colors::LightGray);
		tileMat->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
		tileMat->roughness = 0.2f;

		auto skullMat = std::make_unique<Material>();
		skullMat->name = "mat_skull";
		skullMat->matCBIdx = 3;
		skullMat->diffuseSrvHeapIndex = 3;
		skullMat->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		skullMat->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05);
		skullMat->roughness = 0.3f;

		m_materials["mat_bricks"] = std::move(bricksMat);
		m_materials["mat_stone"] = std::move(stoneMat);
		m_materials["mat_tile"] = std::move(tileMat);
		m_materials["mat_skull"] = std::move(skullMat);
	}

	void Renderer::_createFrameResources()
	{

		for (size_t i = 0; i < FRAME_RESOURCE_COUNT; i++)
		{
			m_frameResources.push_back(
				std::make_unique<FrameResource>(m_device.Get(), 1, 
					m_renderableList.size(), m_materials.size()));
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