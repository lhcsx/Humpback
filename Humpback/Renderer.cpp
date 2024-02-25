// (c) Li Hongcheng
// 2021-10-28

#include <array>
#include <memory>

#include <wrl.h>
#include <dxgi1_6.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>

#include "../DirectXTK12-main/Src/d3dx12.h"
#include "HumpbackHelper.h"
#include "Renderer.h"
#include "D3DUtil.h"
#include "Vertex.h"
#include "GeometryGenetator.h"

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::Colors;


namespace Humpback 
{
	class Material;

	extern const int FRAME_RESOURCE_COUNT = 3;

	std::string_view Renderer::SHADER_MODEL_VERTEX = "vs_5_1";
	std::string_view Renderer::SHADER_MODEL_FRAGMENT = "ps_5_1";

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
		_initCamera();

		OnResize();

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

		_initRendererFeatures();

		_loadTextures();
		_createDescriptorHeaps();
		_createRootSignature();
		_createRootSignatureSSAO();
		_createShadersAndInputLayout();

		_createSceneLights();
		_createSceneGeometry();
		_loadGeometryFromFileASSIMP();
		_createMaterialsData();
		_createRenderableObjects();

		_createFrameResources();
		_createPso();

		m_featureSSAO->SetPSOs(m_psos["ssao"].Get(), m_psos["blur"].Get());

		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* commandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		_waitForPreviousFrame();
	}


	void Renderer::_update()
	{
		_onKeyboardInput();

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

		_updateShadowMap();
		_updateCBuffers();
	}

	void Renderer::_updateCamera()
	{
		m_mainCamera->Update();
	}

	void Renderer::_onKeyboardInput()
	{
		float deltaT = m_timer->DeltaTime();

		if (GetAsyncKeyState('W') & 0x8000)
		{	
			auto pos =  XMLoadFloat3(&m_mainCamera->GetPosition());
			auto newPos = pos + (m_mainCamera->GetForwardVector() * 10.f * deltaT);
			m_mainCamera->SetPosition(newPos);
		}

		if (GetAsyncKeyState('A') & 0x8000)
		{
			auto pos = XMLoadFloat3(&m_mainCamera->GetPosition());
			auto newPos = pos + (m_mainCamera->GetRightVector() * -10.f * deltaT);
			m_mainCamera->SetPosition(newPos);
		}

		if (GetAsyncKeyState('S') & 0x8000)
		{
			auto pos = XMLoadFloat3(&m_mainCamera->GetPosition());
			auto newPos = pos + (m_mainCamera->GetForwardVector() * -10.f * deltaT);
			m_mainCamera->SetPosition(newPos);
		}

		if (GetAsyncKeyState('D') & 0x8000)
		{
			auto pos = XMLoadFloat3(&m_mainCamera->GetPosition());
			auto newPos = pos + (m_mainCamera->GetRightVector() * 10.f * deltaT);
			m_mainCamera->SetPosition(newPos);
		}
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::_getCpuSrv(int idx) const
	{
		auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
		srv.Offset(idx, m_cbvSrvUavDescriptorSize);
		return srv;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE Renderer::_getGpuSrv(int idx) const
	{
		auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		srv.Offset(idx, m_cbvSrvUavDescriptorSize);
		return srv;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::_getDsv(int idx) const
	{
		auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsv.Offset(idx, m_dsvDescriptorSize);
		return dsv;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::_getRtv(int idx) const
	{
		auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtv.Offset(idx, m_rtvDescriptorSize);
		return rtv;
	}

	void Renderer::_bindMaterialBuffer()
	{
		auto matBuffer = m_curFrameResource->materialCBuffer->Resource();
		m_commandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
	}

	void Renderer::_updateCBuffers()
	{
		_updateCBufferPerObject();
		_updateMatCBuffer();
		_updateCBufferPerPass();
		_updateShadowCB();
		_updateSsaoCB();
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
				XMStoreFloat4x4(&(objConstants.worldM), XMMatrixTranspose(worldM));
				if (obj->material != nullptr)
				{
					objConstants.MaterialIndex = obj->material->matCBIdx;
				}

				curObjCBuffer->CopyData(obj->cbIndex, objConstants);

				obj->NumFramesDirty--;
			}
		}
	}

	void Renderer::_updateCBufferPerPass()
	{
		XMMATRIX view = m_mainCamera->GetViewMatrix();
		XMMATRIX proj = m_mainCamera->GetProjectionMatrix();
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
		XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
		XMMATRIX shadowVPT = XMLoadFloat4x4(&m_shadowVPTMatrix);
		
		XMMATRIX viewProjTex = viewProj * DirectX::XMLoadFloat4x4(&HMathHelper::NDCToTexCoord());

		XMStoreFloat4x4(&m_mainPassCB.view, XMMatrixTranspose(view));
		XMStoreFloat4x4(&m_mainPassCB.proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&m_mainPassCB.viewProj, XMMatrixTranspose(viewProj));
		XMStoreFloat4x4(&m_mainPassCB.shadowVPT, XMMatrixTranspose(shadowVPT));
		XMStoreFloat4x4(&m_mainPassCB.invProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&m_mainPassCB.invView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&m_mainPassCB.invViewProj, XMMatrixTranspose(invViewProj));
		XMStoreFloat4x4(&m_mainPassCB.viewProjTex, XMMatrixTranspose(viewProjTex));
		m_mainPassCB.nearZ = m_mainCamera->GetNearZ();
		m_mainPassCB.farZ = m_mainCamera->GetFarZ();
		m_mainPassCB.cameraPosW = m_mainCamera->GetPosition();
		m_mainPassCB.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);

		for (size_t i = 0; i < 3; i++)
		{
			m_mainPassCB.lights[i].direction = m_directionalLights[i].GetDirection();
			m_mainPassCB.lights[i].strength = m_directionalLights[i].GetIntensity();
		}
		
		m_curFrameResource->passCBuffer->CopyData(0, m_mainPassCB);
	}

	void Renderer::_updateMatCBuffer()
	{
		auto curMatCB = m_curFrameResource->materialCBuffer.get();
		for (auto& m : m_materials)
		{
			Material* pMat = m.second.get();

			if (pMat != nullptr && pMat->numFramesDirty > 0)
			{
				
				MaterialConstants matConstants;
				matConstants.diffuseAlbedo = pMat->diffuseAlbedo;

				matConstants.diffuseMapIndex = pMat->diffuseSrvHeapIndex;
				matConstants.normalMapIndex = pMat->normalSrvHeapIndex;
				matConstants.metallicSmothnessMapIndex = pMat->metallicSmothnessSrvHeapIndex;
				
				XMMATRIX matrixMatTrans = XMLoadFloat4x4(&pMat->matTransform);
				XMStoreFloat4x4(&matConstants.matTransform, XMMatrixTranspose(matrixMatTrans));

				curMatCB->CopyData(pMat->matCBIdx, matConstants);

				--pMat->numFramesDirty;
			}
		}
	}

	void Renderer::_updateShadowCB()
	{
		XMMATRIX lightView = XMLoadFloat4x4(&m_lightViewMatrix);
		XMMATRIX lightProj = XMLoadFloat4x4(&m_lightProjMatrix);

		XMMATRIX lightVP = XMMatrixMultiply(lightView, lightProj);

		XMStoreFloat4x4(&m_shadowPassCB.view, XMMatrixTranspose(lightView));
		XMStoreFloat4x4(&m_shadowPassCB.proj, XMMatrixTranspose(lightProj));
		XMStoreFloat4x4(&m_shadowPassCB.viewProj, XMMatrixTranspose(lightVP));
		m_shadowPassCB.cameraPosW = m_mainLightPos;

		m_curFrameResource->passCBuffer->CopyData(1, m_shadowPassCB);
	}

	void Renderer::_updateSsaoCB()
	{
		SSAOConstants constants;
		
		constants.projM = m_mainPassCB.proj;
		constants.invProjM = m_mainPassCB.invProj;

		XMMATRIX p = m_mainCamera->GetProjectionMatrix();
		XMStoreFloat4x4(&constants.projTexM, XMMatrixTranspose(p * XMLoadFloat4x4(&HMathHelper::NDCToTexCoord())));

		m_featureSSAO->GetOffsetVectors(constants.offectVectors);

		auto weights = m_featureSSAO->GetWeights(2.5f);
		constants.weights[0] = XMFLOAT4(&weights[0]);
		constants.weights[1] = XMFLOAT4(&weights[4]);
		constants.weights[2] = XMFLOAT4(&weights[8]);

		float aoTextureWidth = m_featureSSAO->GetAOTextureWidth();
		float aoTextureHeight = m_featureSSAO->GetAOTextureHeight();

		constants.PixelSize = XMFLOAT2(1.0 / aoTextureWidth, 1.0 / aoTextureHeight);

		constants.radius = 0.5f;
		constants.occlusionFadeStart = 0.2f;
		constants.occlusionFadeEnd = 1.0f;
		constants.surfaceEpsilon = 0.05f;

		auto curSsaoCB = m_curFrameResource->ssaoCBuffer.get();
		curSsaoCB->CopyData(0, constants);
	}

	void Renderer::_updateShadowMap()
	{
		// Only the main directional light cast shadow.
		DirectionalLight mainLight = m_directionalLights[0];
		XMVECTOR lightDir = XMLoadFloat3(&mainLight.GetDirection());
		XMVECTOR lightPos = -2.0f * m_sceneBoundingSphere.Radius * lightDir;
		XMVECTOR targetPos = XMLoadFloat3(&m_sceneBoundingSphere.Center);
		XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		XMStoreFloat3(&m_mainLightPos, lightPos);

		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

		float l = sphereCenterLS.x - m_sceneBoundingSphere.Radius;
		float r = sphereCenterLS.x + m_sceneBoundingSphere.Radius;
		float b = sphereCenterLS.y - m_sceneBoundingSphere.Radius;
		float u = sphereCenterLS.y + m_sceneBoundingSphere.Radius;
		float n = sphereCenterLS.z - m_sceneBoundingSphere.Radius;
		float f = sphereCenterLS.z + m_sceneBoundingSphere.Radius;

		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, u, n, f);

		XMMATRIX VPT = lightView * lightProj * XMLoadFloat4x4(&HMathHelper::NDCToTexCoord());
		XMStoreFloat4x4(&m_lightViewMatrix, lightView);
		XMStoreFloat4x4(&m_lightProjMatrix, lightProj);
		XMStoreFloat4x4(&m_shadowVPTMatrix,  VPT);
	}

	void Renderer::_render()
	{
		auto cmdAllocator = m_curFrameResource->cmdAlloc;

		// Reuse the memory associated with command recording.
		// We can only reset the associated command lists have finished execution on the GPU.
		ThrowIfFailed(cmdAllocator->Reset());

		//A command list can be reset after it has been added to the command queue.
		ThrowIfFailed(m_commandList->Reset(cmdAllocator.Get(), m_psos["opaque"].Get()));

		ID3D12DescriptorHeap* srvHeaps[] = {m_srvHeap.Get()};
		m_commandList->SetDescriptorHeaps(_countof(srvHeaps), srvHeaps);

		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		
		_bindMaterialBuffer();

		//m_commandList->SetGraphicsRootDescriptorTable(3, m_nullSrv);

		//m_commandList->SetGraphicsRootDescriptorTable(4, m_srvHeap->GetGPUDescriptorHandleForHeapStart());


		// Shadow map pass.
		//_renderShadowMap();

		// Normal depth pass.
		//_renderNormalDepth();

		// Generate AO.
		//_renderAO();


		// Main pass.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		_bindMaterialBuffer();

		auto backbufferView = _getCurrentBackBufferView();
		auto dsView = _getCurrentDSBufferView();
		m_commandList->OMSetRenderTargets(1, &backbufferView, true, &dsView);

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_getCurrentBackbuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		m_commandList->RSSetViewports(1, &m_viewPort);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Clear depth and color buffers.
		m_commandList->ClearRenderTargetView(_getCurrentBackBufferView(), Colors::DarkGray, 0, nullptr);
		m_commandList->ClearDepthStencilView(_getCurrentDSBufferView(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		m_commandList->SetGraphicsRootDescriptorTable(4, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

		// set skybox root descriptor.
		CD3DX12_GPU_DESCRIPTOR_HANDLE skyDesHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		skyDesHandle.Offset(m_skyTexHeapIndex, m_cbvSrvUavDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(3, skyDesHandle);


		// Bind per-pass constant buffer.
		auto passCB = m_curFrameResource->passCBuffer->Resource();
		m_commandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		// Opaque pass.
		m_commandList->SetPipelineState(m_psos["opaque"].Get());
		_renderRenderableObjects(m_commandList.Get(), m_renderLayers[(int)RenderLayer::Opaque]);

		// Sky box pass.
		m_commandList->SetPipelineState(m_psos["skybox"].Get());
		_renderRenderableObjects(m_commandList.Get(), m_renderLayers[(int)RenderLayer::Sky]);


		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			_getCurrentBackbuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

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

	void Renderer::_renderShadowMap()
	{
		m_commandList->RSSetViewports(1, &(m_shadowMap->GetViewPort()));
		m_commandList->RSSetScissorRects(1, &(m_shadowMap->GetScissorRect()));

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->Resource(), 
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		m_commandList->ClearDepthStencilView(m_shadowMap->DSV(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		m_commandList->OMSetRenderTargets(0, nullptr, false, &m_shadowMap->DSV());

		// Bind shadow map pass constants.
		unsigned int passCBByteSize = D3DUtil::CalConstantBufferByteSize(sizeof(PassConstants));
		auto passCB = m_curFrameResource->passCBuffer->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + passCBByteSize;
		m_commandList->SetGraphicsRootConstantBufferView(1, passCBAddress);
		
		m_commandList->SetPipelineState(m_psos["shadowMap"].Get());

		_renderRenderableObjects(m_commandList.Get(), m_renderLayers[(int)RenderLayer::Opaque]);

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_shadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	void Renderer::_renderNormalDepth()
	{
		m_commandList->RSSetViewports(1, &m_viewPort);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_featureSSAO->GetNormalResource(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

		float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
		m_commandList->ClearRenderTargetView(m_featureSSAO->GetNormalRTV(), clearValue, 0, nullptr);

		m_commandList->ClearDepthStencilView(_getCurrentDSBufferView(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		m_commandList->OMSetRenderTargets(1, &m_featureSSAO->GetNormalRTV(), true, &_getCurrentDSBufferView());

		auto passCB = m_curFrameResource->passCBuffer->Resource();
		m_commandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		m_commandList->SetPipelineState(m_psos["normalDepth"].Get());

		_renderRenderableObjects(m_commandList.Get(), m_renderLayers[(int)RenderLayer::Opaque]);

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_featureSSAO->GetNormalResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	void Renderer::_renderAO()
	{
		m_commandList->SetGraphicsRootSignature(m_rootSignatureSSAO.Get());
		m_featureSSAO->Execute(m_commandList.Get(), m_curFrameResource, 3);
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
		fbClear.DepthStencil.Stencil = 0;
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

		if (m_featureSSAO != nullptr)
		{
			m_featureSSAO->OnResize(m_width, m_height);
			m_featureSSAO->RebuildDescriptors(m_depthStencilBuffer.Get());
		}

		m_mainCamera->SetFrustum(0.25f * HMathHelper::PI, m_aspectRatio, 1.0f, 1000.0f);
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
		if((btnState & MK_RBUTTON) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePoint.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePoint.y));

			m_mainCamera->Pitch(dy);
			m_mainCamera->RotateY(dx);
		}

		m_lastMousePoint.x = x;
		m_lastMousePoint.y = y;
	}

	void Renderer::OnMouseWheel(short delta)
	{
		auto pos = XMLoadFloat3(&m_mainCamera->GetPosition());
		auto newPos = pos + (m_mainCamera->GetForwardVector() * delta * 0.05f);
		m_mainCamera->SetPosition(newPos);
	}

	void Renderer::_cleanUp()
	{
		m_timer.reset();
		m_timer.release();
		if (m_modelLoader)
		{
			m_modelLoader.reset();
		}
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

	void Renderer::_initCamera()
	{
		_createCamera();
		m_mainCamera->SetPosition(0.0f, 2.0f, -15.0f);
	}

	void Renderer::_createCamera()
	{
		m_mainCamera = std::make_unique<Camera>();
	}

	void Renderer::_createSceneGeometry()
	{
		// Build the bounding sphere of the scene.
		m_sceneBoundingSphere.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_sceneBoundingSphere.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

		_createSimpleGeometry();
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
			vertices[k].uv = box.vertices[i].TexC;
			vertices[k].tangent = box.vertices[i].TangentU;
		}

		for (size_t i = 0; i < grid.vertices.size(); ++i, ++k)
		{
			vertices[k].position = grid.vertices[i].Position;
			vertices[k].normal = grid.vertices[i].Normal;
			vertices[k].uv = grid.vertices[i].TexC;
			vertices[k].tangent = grid.vertices[i].TangentU;
		}

		for (size_t i = 0; i < sphere.vertices.size(); ++i, ++k)
		{
			vertices[k].position = sphere.vertices[i].Position;
			vertices[k].normal = sphere.vertices[i].Normal;
			vertices[k].uv = sphere.vertices[i].TexC;
			vertices[k].tangent = sphere.vertices[i].TangentU;
		}

		for (size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
		{
			vertices[k].position = cylinder.vertices[i].Position;
			vertices[k].normal = cylinder.vertices[i].Normal;
			vertices[k].uv = cylinder.vertices[i].TexC;
			vertices[k].tangent = cylinder.vertices[i].TangentU;
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

		XMFLOAT3 vMinF(FLT_MAX, FLT_MAX, FLT_MAX);
		XMFLOAT3 vMaxF(-FLT_MIN, -FLT_MAX, -FLT_MAX);

		XMVECTOR vMin = XMLoadFloat3(&vMinF);
		XMVECTOR vMax = XMLoadFloat3(&vMaxF);

		std::vector<Vertex> vertices(vertCount);
		for (size_t i = 0; i < vertCount; i++)
		{
			fin >> vertices[i].position.x >> vertices[i].position.y >> vertices[i].position.z;
			fin >> vertices[i].normal.x >> vertices[i].normal.y >> vertices[i].normal.z;

			XMVECTOR p = XMLoadFloat3(&vertices[i].position);

			XMVECTOR N = XMLoadFloat3(&vertices[i].normal);

			// Build the tangent.
			XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f)
			{
				XMVECTOR tangent = XMVector3Normalize(XMVector3Cross(up, N));
				XMStoreFloat3(&vertices[i].tangent, tangent);
			}
			else
			{
				up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
				XMVECTOR tangent = XMVector3Normalize(XMVector3Cross(N, up));
				XMStoreFloat3(&vertices[i].tangent, tangent);
			}

			vMin = XMVectorMin(p, vMin);
			vMax = XMVectorMax(p, vMax);
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

		BoundingBox aabb;
		XMStoreFloat3(&aabb.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&aabb.Extents, 0.5f * (vMax - vMin));

		skullSM.aabb = aabb;

		skullMesh->drawArgs["skull"] = skullSM;

		m_meshes[skullMesh->Name] = std::move(skullMesh);
	}

	void Renderer::_loadGeometryFromFileASSIMP()
	{
		m_modelLoader = std::make_unique<HMeshImporter>(m_device.Get(), m_commandList.Get());

		if (m_modelLoader->Load("Assets/PreviewSphere.fbx") == false)
		{
			MessageBox(0, L"Assets/PreviewSphere.fbx", 0, 0);
		}


	}

	void Renderer::_createSceneLights()
	{
		m_directionalLights = std::make_unique<DirectionalLight[]>(3);
		m_directionalLights[0].SetDirection(0.57735f, -0.57735f, 0.57735f);
		m_directionalLights[0].SetIntensity(2.f, 2.f, 2.f);

		m_directionalLights[1].SetDirection(-0.57735f, -0.57735f, 0.57735f);
		m_directionalLights[1].SetIntensity(0.3f, 0.3f, 0.3f);

		m_directionalLights[2].SetDirection(0.0f, -0.707f, -0.707f);
		m_directionalLights[2].SetIntensity(0.15f, 0.15f, 0.15f);
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
		m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		
		_createCommandObjects();
		_createSwapChain(factory.Get());
	}

	void Renderer::_initRendererFeatures()
	{
		m_shadowMap = std::make_unique<ShadowMap>(m_device.Get(), 2048, 2048);
		m_featureSSAO = std::make_unique<SSAO>(m_width, m_height, m_device.Get(), m_commandList.Get());
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
		rtvDesc.NumDescriptors = FrameBufferCount + 3;	// A normal map and two AO maps.
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
		CD3DX12_DESCRIPTOR_RANGE texTable0;
		texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);

		CD3DX12_DESCRIPTOR_RANGE texTable1;
		texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 3, 0);

		CD3DX12_ROOT_PARAMETER slotRootParameter[5];

		slotRootParameter[0].InitAsConstantBufferView(0);
		slotRootParameter[1].InitAsConstantBufferView(1);
		slotRootParameter[2].InitAsShaderResourceView(0, 1);
		slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

		auto staticSamplers = D3DUtil::GetCommonStaticSamplers();

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, staticSamplers.size(),
			staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

		_createRootSignatureSSAO();
	}

	void Renderer::_createRootSignatureSSAO()
	{
		CD3DX12_DESCRIPTOR_RANGE texTable0;
		texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

		CD3DX12_DESCRIPTOR_RANGE texTable1;
		texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

		CD3DX12_ROOT_PARAMETER rootParams[4];
		rootParams[0].InitAsConstantBufferView(0);
		rootParams[1].InitAsConstants(1, 1);
		rootParams[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParams[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

		const CD3DX12_STATIC_SAMPLER_DESC pointClamp(0,
			D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		const CD3DX12_STATIC_SAMPLER_DESC linearClamp(1,
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		const CD3DX12_STATIC_SAMPLER_DESC depthSamp(2,
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.0f, 0, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(3,
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP);

		std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamps = {
			pointClamp, linearClamp, depthSamp, linearWrap };

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc(4, rootParams, staticSamps.size(), 
			staticSamps.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorMsg = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorMsg.GetAddressOf());

		if (errorMsg != nullptr)
		{
			::OutputDebugStringA((char*)errorMsg->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatureSSAO)));
	}

	void Renderer::_createShadersAndInputLayout()
	{
		std::wstring pbrFullPath = GetAssetPath(L"\\shaders\\StandardPBR.hlsl");
		_createVertexShader(pbrFullPath, "standard_pbr_vs");
		_createPixelShader(pbrFullPath, "standard_pbr_ps");

		std::wstring skyBoxShaderFullPath = GetAssetPath(L"\\shaders\\Sky.hlsl");
		_createVertexShader(skyBoxShaderFullPath, "skyBoxVS");
		_createPixelShader(skyBoxShaderFullPath, "skyBoxPS");

		std::wstring shadowMapShaderFullPath = GetAssetPath(L"\\shaders\\ShadowMap.hlsl");
		_createVertexShader(shadowMapShaderFullPath, "shadowMapVS");

		std::wstring ssaoShaderFullPath = GetAssetPath(L"\\shaders\\SSAO.hlsl");
		_createVertexShader(ssaoShaderFullPath, "ssaoVS");
		_createPixelShader(ssaoShaderFullPath, "ssaoPS");

		std::wstring blurShaderFullPath = GetAssetPath(L"\\shaders\\Blur.hlsl");
		_createVertexShader(blurShaderFullPath, "blurVS");
		_createPixelShader(blurShaderFullPath, "blurPS");

		std::wstring normalOnlyFullPath = GetAssetPath(L"\\shaders\\NormalOnly.hlsl");
		_createVertexShader(normalOnlyFullPath, "normalOnlyVS");
		_createPixelShader(normalOnlyFullPath, "normalOnlyPS");

		m_inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
	}

	void Renderer::_createPso()
	{
		// -----------------------------------------------------------------------------
		// PSO for opaque objects.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
		ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		opaquePsoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
		opaquePsoDesc.pRootSignature = m_rootSignature.Get();
		opaquePsoDesc.VS =
		{
			reinterpret_cast<BYTE*>(m_shaders["standard_pbr_vs"]->GetBufferPointer()),
			m_shaders["standard_pbr_vs"]->GetBufferSize()
		};
		opaquePsoDesc.PS =
		{
			reinterpret_cast<BYTE*>(m_shaders["standard_pbr_ps"]->GetBufferPointer()),
			m_shaders["standard_pbr_ps"]->GetBufferSize()
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
		// --------------------------------------------------------------------------------
		
		// PSO for sky box.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC skyBoxPSODesc = opaquePsoDesc;

		skyBoxPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		skyBoxPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		skyBoxPSODesc.pRootSignature = m_rootSignature.Get();

		skyBoxPSODesc.VS =
		{
			reinterpret_cast<BYTE*>(m_shaders["skyBoxVS"]->GetBufferPointer()),
			m_shaders["skyBoxVS"]->GetBufferSize()
		};
		skyBoxPSODesc.PS = 
		{
			reinterpret_cast<BYTE*>(m_shaders["skyBoxPS"]->GetBufferPointer()),
			m_shaders["skyBoxPS"]->GetBufferSize()
		};

		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&skyBoxPSODesc, IID_PPV_ARGS(&m_psos["skybox"])));

		// PSO for shadow map.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC sm_PSO_desc = opaquePsoDesc;
		sm_PSO_desc.VS =
		{
			reinterpret_cast<BYTE*>(m_shaders["shadowMapVS"]->GetBufferPointer()),
			m_shaders["shadowMapVS"]->GetBufferSize()
		};
		sm_PSO_desc.PS =
		{
			nullptr,
			0
		};

		sm_PSO_desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		sm_PSO_desc.NumRenderTargets = 0;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&sm_PSO_desc, IID_PPV_ARGS(&m_psos["shadowMap"])));

		// PSO for normal-depth pass.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC normalDepthPsoDesc = opaquePsoDesc;
		normalDepthPsoDesc.VS = 
		{
			reinterpret_cast<BYTE*>(m_shaders["normalOnlyVS"]->GetBufferPointer()),
			m_shaders["normalOnlyVS"]->GetBufferSize()
		};
		normalDepthPsoDesc.PS = 
		{
			reinterpret_cast<BYTE*>(m_shaders["normalOnlyPS"]->GetBufferPointer()),
			m_shaders["normalOnlyPS"]->GetBufferSize()
		};
		normalDepthPsoDesc.RTVFormats[0] = SSAO::NORMAL_DEPTH_FORMAT;
		normalDepthPsoDesc.SampleDesc.Count = 1;
		normalDepthPsoDesc.SampleDesc.Quality = 0;
		normalDepthPsoDesc.DSVFormat = m_dsFormat;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&normalDepthPsoDesc, IID_PPV_ARGS(&m_psos["normalDepth"])));
		

		// PSO for SSAO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = opaquePsoDesc;
		ssaoPsoDesc.InputLayout = { nullptr, 0 };
		ssaoPsoDesc.pRootSignature = m_rootSignatureSSAO.Get();
		ssaoPsoDesc.VS = 
		{
			reinterpret_cast<byte*>(m_shaders["ssaoVS"]->GetBufferPointer()),
			m_shaders["ssaoVS"]->GetBufferSize()
		};
		ssaoPsoDesc.PS = 
		{
			reinterpret_cast<byte*>(m_shaders["ssaoPS"]->GetBufferPointer()),
			m_shaders["ssaoPS"]->GetBufferSize()
		};

		ssaoPsoDesc.DepthStencilState.DepthEnable = false;
		ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		ssaoPsoDesc.RTVFormats[0] = SSAO::AMBIENT_FORMAT;
		ssaoPsoDesc.SampleDesc.Count = 1;
		ssaoPsoDesc.SampleDesc.Quality = 0;
		ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&m_psos["ssao"])));

		// PSO for blur pass.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC blurPsoDesc = ssaoPsoDesc;
		blurPsoDesc.VS =
		{
			reinterpret_cast<byte*>(m_shaders["blurVS"]->GetBufferPointer()),
			m_shaders["blurVS"]->GetBufferSize()
		};
		blurPsoDesc.PS =
		{
			reinterpret_cast<byte*>(m_shaders["blurPS"]->GetBufferPointer()),
			m_shaders["blurPS"]->GetBufferSize()
		};
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&blurPsoDesc, IID_PPV_ARGS(&m_psos["blur"])));
	}

	void Renderer::_createVertexShader(const std::wstring& fullPath, const std::string& shaderName)
	{
		m_shaders[shaderName] = D3DUtil::CompileShader(fullPath, nullptr, "VS", Renderer::SHADER_MODEL_VERTEX);
	}

	void Renderer::_createPixelShader(const std::wstring& fullPath, const std::string& shaderName)
	{
		m_shaders[shaderName] = D3DUtil::CompileShader(fullPath, nullptr, "PS", Renderer::SHADER_MODEL_FRAGMENT);
	}

	void Renderer::_createRenderableObjects()
	{
		int constantBufferIdx = 0;

		auto sky = std::make_unique<RenderableObject>();
		XMStoreFloat4x4(&sky->worldM, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
		sky->texTrans = HMathHelper::Identity4x4();
		sky->cbIndex = constantBufferIdx;
		sky->material = m_materials["mat_sky"].get();
		sky->mesh = m_meshes["shapeGeo"].get();
		sky->indexCount = sky->mesh->drawArgs["sphere"].indexCount;
		sky->startIndexLocation = sky->mesh->drawArgs["sphere"].startIndexLocation;
		sky->baseVertexLocation = sky->mesh->drawArgs["sphere"].baseVertexLocation;
		m_renderLayers[(int)RenderLayer::Sky].push_back(sky.get());
		m_renderableList.push_back(std::move(sky));
		++constantBufferIdx;

		auto gridRitem = std::make_unique<RenderableObject>();
		gridRitem->worldM = HMathHelper::Identity4x4();
		XMStoreFloat4x4(&gridRitem->texTrans, XMMatrixScaling(8.0f, 8.0f, 1.0f));
		gridRitem->cbIndex = constantBufferIdx;
		gridRitem->material = m_materials["mat_bricks"].get();
		gridRitem->mesh = m_meshes["shapeGeo"].get();
		gridRitem->indexCount = gridRitem->mesh->drawArgs["grid"].indexCount;
		gridRitem->startIndexLocation = gridRitem->mesh->drawArgs["grid"].startIndexLocation;
		gridRitem->baseVertexLocation = gridRitem->mesh->drawArgs["grid"].baseVertexLocation;
		m_renderLayers[(int)RenderLayer::Opaque].push_back(gridRitem.get());
		m_renderableList.push_back(std::move(gridRitem));
		++constantBufferIdx;

		auto previewSphereItem = std::make_unique<RenderableObject>();
		XMStoreFloat4x4(&previewSphereItem->worldM, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(2.0f, 2.5f, 0.0f));
		previewSphereItem->texTrans = HMathHelper::Identity4x4();
		previewSphereItem->cbIndex = constantBufferIdx;
		previewSphereItem->material = m_materials["mat_sphere"].get();
		previewSphereItem->mesh = m_modelLoader->GetMesh();
		previewSphereItem->instanceCount = 0;
		previewSphereItem->indexCount = previewSphereItem->mesh->drawArgs["main"].indexCount;
		previewSphereItem->startIndexLocation = previewSphereItem->mesh->drawArgs["main"].startIndexLocation;
		previewSphereItem->startIndexLocation = previewSphereItem->mesh->drawArgs["main"].baseVertexLocation;
		m_renderLayers[(int)RenderLayer::Opaque].push_back(previewSphereItem.get());
		m_renderableList.push_back(std::move(previewSphereItem));
		++constantBufferIdx;
	}

	void Renderer::_createMaterialsData()
	{
		int matCbIdx = 0;

		auto bricksMat = std::make_unique<Material>();
		bricksMat->name = "mat_bricks";
		bricksMat->matCBIdx = matCbIdx;
		bricksMat->diffuseSrvHeapIndex = 0;
		bricksMat->normalSrvHeapIndex = 1;
		bricksMat->metallicSmothnessSrvHeapIndex = 2;
		bricksMat->diffuseAlbedo = XMFLOAT4(Colors::LightGray);
		++matCbIdx;

		auto skyMat = std::make_unique<Material>();
		skyMat->name = "mat_sky";
		skyMat->matCBIdx = matCbIdx;
		skyMat->diffuseSrvHeapIndex = 3;
		skyMat->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		++matCbIdx;

		auto previewSphereMat = std::make_unique<Material>();
		previewSphereMat->name = "mat_preview_sphere";
		previewSphereMat->matCBIdx = matCbIdx;
		previewSphereMat->diffuseSrvHeapIndex = 0;
		previewSphereMat->normalSrvHeapIndex = 1;
		previewSphereMat->metallicSmothnessSrvHeapIndex = 2;
		previewSphereMat->diffuseAlbedo = XMFLOAT4(0.5f, 0.f, 0.f, 1.0f);
		++matCbIdx;

		m_materials["mat_bricks"] = std::move(bricksMat);
		m_materials["mat_sky"] = std::move(skyMat);
		m_materials["mat_sphere"] = std::move(previewSphereMat);
	}

	void Renderer::_loadTextures()
	{
		std::vector<std::string> texNames =
		{
			"tex_sphere_albedo",
			"tex_sphere_normal",
			"tex_sphere_metallic",
		};

		std::vector<std::wstring> texPaths =
		{
			L"Assets/PreviewSphere_Sphere_AlbedoTransparency.png",
			L"Assets/PreviewSphere_Sphere_Normal.png",
			L"Assets/PreviewSphere_Sphere_MetallicSmoothness.png",
		};

		m_defaultNormalMapIndex = 4;



		// Create the skybox cubemap from the dds texture file.
		{
			auto tex = std::make_unique<Texture>();
			tex->name = "sky_box";
			ThrowIfFailed(CreateDDSTextureFromFile12(m_device.Get(), m_commandList.Get(), L"Assets/grasscube1024.dds",
				tex->resource, tex->uploadHeap));

			m_textures[tex->name] = std::move(tex);
		}

		for (size_t i = 0; i < texPaths.size(); i++)
		{
			std::wstring path = texPaths[i];
			auto tex = std::make_unique<Texture>();
			tex->name = texNames[i];
			tex->filePath = texPaths[i];
			D3D12_SUBRESOURCE_DATA subresource;
			std::unique_ptr<uint8_t[]> decodedData;
			ThrowIfFailed(LoadWICTextureFromFile(m_device.Get(), path.c_str(), 
				tex->resource.GetAddressOf(), decodedData, subresource, 2048));

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(tex->resource.Get(), 0, 1);

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

			auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

			// Create the GPU upload buffer.
			ThrowIfFailed(
				m_device->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(tex->uploadHeap.GetAddressOf())));
		
			UpdateSubresources<1>(m_commandList.Get(), tex->resource.Get(), tex->uploadHeap.Get(),
				0, 0, 1, &subresource);

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(tex->resource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			m_commandList->ResourceBarrier(1, &barrier);

			m_textures[tex->name] = std::move(tex);
		}
	}

	void Renderer::_createDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = 18;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvDescHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());

		std::vector<ComPtr<ID3D12Resource>> tex2DList = {
			m_textures["tex_sphere_albedo"]->resource,
			m_textures["tex_sphere_normal"]->resource,
			m_textures["tex_sphere_metallic"]->resource,
		};


		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		for (size_t i = 0; i < tex2DList.size(); i++)
		{
			// Create srv for diffuse and normal textures.
			srvDesc.Format = tex2DList[i]->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
			m_device->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, srvDescHandle);

			srvDescHandle.Offset(1, m_cbvSrvUavDescriptorSize);
		}

		// Create srv for sky box.
		auto skyCubeMap = m_textures["sky_box"]->resource;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
		srvDesc.Format = skyCubeMap->GetDesc().Format;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		m_device->CreateShaderResourceView(skyCubeMap.Get(), &srvDesc, srvDescHandle);

		m_skyTexHeapIndex = tex2DList.size();
		//m_shadowMapHeapIndex = tex2DList.size() + 1;
		//m_ssaoHeapIndexStart = m_shadowMapHeapIndex + 1;
		//m_ssaoAmbientMapIndex = m_ssaoHeapIndexStart + 3;
		//int nullCubeSrvIndex = m_ssaoHeapIndexStart + 5;
		//int nullTexSrvIndex1 = nullCubeSrvIndex + 1;
		//int nullTexSrvIndex2 = nullTexSrvIndex1 + 1;

		//auto nullSrv = _getCpuSrv(nullCubeSrvIndex);
		//m_nullSrv = _getGpuSrv(nullCubeSrvIndex);

		//m_device->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
		//nullSrv.Offset(1, m_cbvSrvUavDescriptorSize);

		//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		//srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//srvDesc.Texture2D.MostDetailedMip = 0;
		//srvDesc.Texture2D.MipLevels = 1;
		//srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;

		//// Create srv for null shadow map.
		//m_device->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

		//nullSrv.Offset(1, m_cbvSrvUavDescriptorSize);
		//m_device->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

		//m_shadowMap->BuildDescriptors(_getCpuSrv(m_shadowMapHeapIndex),
		//	_getGpuSrv(m_shadowMapHeapIndex), _getDsv(1));

		//m_featureSSAO->BuildDescriptors(m_depthStencilBuffer.Get(),
		//	_getCpuSrv(m_ssaoHeapIndexStart), _getGpuSrv(m_ssaoHeapIndexStart),
		//	_getRtv(Renderer::FrameBufferCount), m_cbvSrvUavDescriptorSize, m_rtvDescriptorSize);
	}

	void Renderer::_createFrameResources()
	{
		for (size_t i = 0; i < FRAME_RESOURCE_COUNT; i++)
		{
			m_frameResources.push_back(
				std::make_unique<FrameResource>(m_device.Get(), 2, 
					m_renderableList.size(), m_instanceCount, m_materials.size()));
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