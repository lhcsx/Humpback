// (c) Li Hongcheng
// 2023-05-01


#include <DirectXPackedVector.h>

#include "SSAO.h"


using namespace DirectX;
using namespace DirectX::PackedVector;


namespace Humpback
{
	SSAO::SSAO(UINT width, UINT height, ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList)
	{
		m_device = pDevice;

		_onResize(width, height);

		_buildOffsetVectors();
		_buildRandomVectorTex(cmdList);
	}

	void SSAO::SetPSOs(ID3D12PipelineState* ssaoPSO, ID3D12PipelineState* blurPSO)
	{
		m_SSAOPipelineState = ssaoPSO;
		m_blurPipelineState = blurPSO;
	}

	void SSAO::OnResize(unsigned int newWidth, unsigned int newHeight)
	{
		if (m_width != newWidth || m_height != newHeight)
		{
			m_viewPort.TopLeftX = .0f;
			m_viewPort.TopLeftY = .0f;
			m_viewPort.Width = newWidth / 2;
			m_viewPort.Height = newHeight / 2;
			m_viewPort.MinDepth = .0f;
			m_viewPort.MaxDepth = 1.0f;

			m_width = newWidth;
			m_height = newHeight;

			m_scissorRect = { 0, 0, (int)m_width / 2, (int)m_height / 2, };

			_buildResources();
		}
	}

	void SSAO::ComputeSSAO(ID3D12GraphicsCommandList* cmdList, FrameResource* curFrame, int blurCount)
	{
		cmdList->RSSetViewports(1, &m_viewPort);
		cmdList->RSSetScissorRects(1, &m_scissorRect);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOTexture0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

		cmdList->OMSetRenderTargets(1, &m_SSAOTex0CPURtv, true, nullptr);

		float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		cmdList->ClearRenderTargetView(m_SSAOTex0CPURtv, clearValue, 0, nullptr);

		auto cBufferAddress = curFrame->ssaoCBuffer->Resource()->GetGPUVirtualAddress();
		cmdList->SetGraphicsRootConstantBufferView(0, cBufferAddress);
		cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);

		cmdList->SetGraphicsRootDescriptorTable(2, m_normalDepthGPUSrv);
		cmdList->SetGraphicsRootDescriptorTable(3, m_randomVectorGPUSrv);

		cmdList->SetPipelineState(m_SSAOPipelineState);

		cmdList->IASetVertexBuffers(0, 0, nullptr);
		cmdList->IASetIndexBuffer(nullptr);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->DrawInstanced(6, 1, 0, 0);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOTexture0.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

		_doBlur(cmdList, blurCount, curFrame);
	}

	void SSAO::Execute(ID3D12GraphicsCommandList* cmdList, FrameResource* pCurFrameRes, int blurCount)
	{
		if (cmdList == nullptr)
		{
			return;
		}

		_setUp(cmdList, pCurFrameRes);
		
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOTexture0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
		
		_drawFullScreenQuad(cmdList);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOTexture0.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

		_doBlur(cmdList, blurCount, pCurFrameRes);
	}

	void SSAO::RebuildDescriptors(ID3D12Resource* depthStencilBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = NORMAL_DEPTH_FORMAT;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		m_device->CreateShaderResourceView(m_normalDepthTexture.Get(), &srvDesc, m_normalDepthCpuSrv);

		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		m_device->CreateShaderResourceView(depthStencilBuffer, &srvDesc, m_depthTexCpuSrv);

		srvDesc.Format = AMBIENT_FORMAT;
		m_device->CreateShaderResourceView(m_SSAOTexture0.Get(), &srvDesc, m_SSAOTex0CPUSrv);
		m_device->CreateShaderResourceView(m_SSAOTexture1.Get(), &srvDesc, m_SSAOTex1CPUSrv);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = NORMAL_DEPTH_FORMAT;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		m_device->CreateRenderTargetView(m_normalDepthTexture.Get(), &rtvDesc, m_normalDepthCpuRtv);

		rtvDesc.Format = AMBIENT_FORMAT;
		m_device->CreateRenderTargetView(m_SSAOTexture0.Get(), &rtvDesc, m_SSAOTex0CPURtv);
		m_device->CreateRenderTargetView(m_SSAOTexture1.Get(), &rtvDesc, m_SSAOTex1CPURtv);
	}

	void SSAO::GetOffsetVectors(DirectX::XMFLOAT4 offsets[])
	{
		std::copy(&m_offsets[0], &m_offsets[14], &offsets[0]);
	}

	std::vector<float> SSAO::GetWeights(float sigma)
	{
		int blurRadius = (int)ceil(2.0f * sigma);

		std::vector<float> weights;
		weights.resize(2 * blurRadius + 1);

		float sum = .0f;
		float sigma2 = 2.0f * sigma * sigma;
		for (int i = -blurRadius; i < blurRadius; i++)
		{
			float x = (float)i;
			weights[i + blurRadius] = expf(-x * x / sigma2);
			sum += weights[i + blurRadius];
		}

		// Normalize.
		for (int i = 0; i < weights.size(); i++)
		{
			weights[i] /= sum;
		}

		return weights;
	}

	float SSAO::GetAOTextureWidth()
	{
		return m_width;
	}

	float SSAO::GetAOTextureHeight()
	{
		return m_height;
	}

	void SSAO::_setUp(ID3D12GraphicsCommandList* cmdList, FrameResource* pCurFrameRes)
	{
		cmdList->RSSetViewports(1, &m_viewPort);
		cmdList->RSSetScissorRects(1, &m_scissorRect);
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		cmdList->ClearRenderTargetView(m_SSAOTex0CPUSrv, clearColor, 0, nullptr);

		cmdList->OMSetRenderTargets(1, &m_SSAOTex0CPUSrv, true, nullptr);
		auto cbAdress = pCurFrameRes->ssaoCBuffer->Resource()->GetGPUVirtualAddress();
		cmdList->SetGraphicsRootConstantBufferView(0, cbAdress);

		cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
		cmdList->SetGraphicsRootDescriptorTable(2, m_normalDepthGPUSrv);
		cmdList->SetGraphicsRootDescriptorTable(3, m_randomVectorGPUSrv);

		cmdList->SetPipelineState(m_SSAOPipelineState);
	}



	void SSAO::_doBlur(ID3D12GraphicsCommandList* cmdList, int blurCount, FrameResource* frameRes)
	{
		cmdList->SetPipelineState(m_blurPipelineState);

		auto ssaoCBAddress = frameRes->ssaoCBuffer->Resource()->GetGPUVirtualAddress();
		cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

		for (size_t i = 0; i < blurCount; i++)
		{
			_doBlur(cmdList, true);
			_doBlur(cmdList, false);
		}
	}

	void SSAO::_onResize(unsigned int width, unsigned int height)
	{
		if (m_width == width && m_height == height)
		{
			return;
		}

		m_width = width;
		m_height = height;

		m_viewPort.TopLeftX = 0.0f;
		m_viewPort.TopLeftY = 0.0f;
		m_viewPort.Width = m_width / 2;
		m_viewPort.Height = m_height / 2;
		m_viewPort.MinDepth = 0.0f;
		m_viewPort.MaxDepth = 1.0f;

		m_scissorRect = { 0, 0, (int)m_width / 2, (int)m_height / 2 };

		_buildResources();
	}

	void SSAO::_buildResources()
	{
		m_normalDepthTexture = nullptr;
		m_SSAOTexture0 = nullptr;
		m_SSAOTexture1 = nullptr;

		// Normal depth texture.
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Format = NORMAL_DEPTH_FORMAT;
		texDesc.MipLevels = 1;
		texDesc.Alignment = 0;
		texDesc.DepthOrArraySize = 1;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Width = m_width;
		texDesc.Height = m_height;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		float normalDepthClearVal[] = { 0.0f, 0.0f, 1.0f, 0.0f };
		CD3DX12_CLEAR_VALUE optClear(NORMAL_DEPTH_FORMAT, normalDepthClearVal);
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &optClear,
			IID_PPV_ARGS(&m_normalDepthTexture)));

		// AO texture.
		texDesc.Width = m_width / 2;
		texDesc.Height = m_height / 2;
		texDesc.Format = AMBIENT_FORMAT;

		float ambientClearVal[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		optClear = CD3DX12_CLEAR_VALUE(AMBIENT_FORMAT, ambientClearVal);
		
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &optClear,
			IID_PPV_ARGS(&m_SSAOTexture0)));

		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &optClear,
			IID_PPV_ARGS(&m_SSAOTexture1)));
	}

	void SSAO::_buildOffsetVectors()
	{
		// 6 corners
		m_offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
		m_offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

		m_offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
		m_offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

		m_offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
		m_offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

		m_offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
		m_offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

		// 6 centers
		m_offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
		m_offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

		m_offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
		m_offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

		m_offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
		m_offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

		// randomize
		for (size_t i = 0; i < 14; i++)
		{
			float scale = HMathHelper::RandF(0.25f, 1.0f);
			
			XMVECTOR normV = XMVector4Normalize(XMLoadFloat4(&m_offsets[i]));
			
			XMStoreFloat4(&m_offsets[i], normV * scale);
		}
	}

	void SSAO::_buildRandomVectorTex(ID3D12GraphicsCommandList* cmdList)
	{
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = 256;
		texDesc.Height = 256;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_randomVectorTex)
		));


		UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_randomVectorTex.Get(), 0, 1);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_randomVectorTexUpload)
		));

		XMCOLOR texData[256 * 256];
		for (size_t i = 0; i < 256; i++)
		{
			for (size_t j = 0; j < 256; j++)
			{
				texData[i * 256 + j] = 
					XMCOLOR(HMathHelper::RandF(), HMathHelper::RandF(), HMathHelper::RandF(), 0.0f);
			}
		}

		D3D12_SUBRESOURCE_DATA resData = {};
		resData.pData = texData;
		resData.RowPitch = 256 * sizeof(XMCOLOR);
		resData.SlicePitch = resData.RowPitch * 256;

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_randomVectorTex.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

		UpdateSubresources(cmdList, m_randomVectorTex.Get(), 
			m_randomVectorTexUpload.Get(), 0, 0, 1, &resData);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_randomVectorTex.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	void SSAO::BuildDescriptors(
		ID3D12Resource* depthStencilBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		unsigned int cbvSrvUavDescriptorSize, unsigned int rtvDescriptorSize)
	{
		m_SSAOTex0CPUSrv = hCpuRtv;
		m_SSAOTex1CPUSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
		m_normalDepthCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
		m_depthTexCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
		m_randomVectorCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);

		m_SSAOTex0GPUSrv = hGpuSrv;
		m_SSAOTex1GPUSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
		m_normalDepthGPUSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
		m_depthTexGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
		m_randomVectorGPUSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);

		m_normalDepthCpuRtv = hCpuRtv;
		m_SSAOTex0CPURtv = hCpuRtv.Offset(1, rtvDescriptorSize);
		m_SSAOTex1CPURtv = hCpuRtv.Offset(1, rtvDescriptorSize);

		RebuildDescriptors(depthStencilBuffer);
	}

	void SSAO::_doBlur(ID3D12GraphicsCommandList* cmdList, bool isHorizontal)
	{
		ID3D12Resource* output = nullptr;
		CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
		CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

		if (isHorizontal)
		{
			output = m_SSAOTexture1.Get();
			inputSrv = m_SSAOTex0GPUSrv;
			outputRtv = m_SSAOTex1CPURtv;
			cmdList->SetGraphicsRoot32BitConstant(1, 1, 0);
		}
		else
		{
			output = m_SSAOTexture1.Get();
			inputSrv = m_SSAOTex1GPUSrv;
			outputRtv = m_SSAOTex0CPURtv;
			cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
		}

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output, 
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		cmdList->ClearRenderTargetView(outputRtv, clearColor, 0, nullptr);

		cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

		cmdList->SetGraphicsRootDescriptorTable(2, m_normalDepthGPUSrv);

		cmdList->SetGraphicsRootDescriptorTable(3, inputSrv); // Bind input texture.

		_drawFullScreenQuad(cmdList);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output, 
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	void SSAO::_drawFullScreenQuad(ID3D12GraphicsCommandList* cmdList)
	{
		cmdList->IASetVertexBuffers(0, 0, nullptr);
		cmdList->IASetIndexBuffer(nullptr);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->DrawInstanced(6, 1, 0, 0);
	}
}