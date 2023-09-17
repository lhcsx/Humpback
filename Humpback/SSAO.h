// (c) Li Hongcheng
// 2023-05-01


#pragma once

#include "D3DUtil.h"
#include "FrameResource.h"


namespace Humpback
{
	class SSAO
	{
	public:

		SSAO(UINT width, UINT height, ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList);

		SSAO(const SSAO& rhs) = delete;
		SSAO& operator=(const SSAO& rhs) = delete;

		void SetPSOs();
		void OnResize();
		void ComputeSSAO();

		void Execute(ID3D12GraphicsCommandList* cmdList, FrameResource* pCurFrameRes, int blurCount);

		void RebuildDescriptors(ID3D12Resource* depthStencilBuffer);

	private:

		const DXGI_FORMAT NORMAL_DEPTH_FORMAT = DXGI_FORMAT_R16G16B16A16_FLOAT;
		const DXGI_FORMAT AMBIENT_FORMAT = DXGI_FORMAT_R16_UNORM;

		void _setUp(ID3D12GraphicsCommandList* cmdList, FrameResource* pCurFrameRes);
		void _drawFullScreenQuad(ID3D12GraphicsCommandList* cmdList);
		
		void _doBlur(ID3D12GraphicsCommandList* cmdList, bool isHorizontal);
		void _doBlur(ID3D12GraphicsCommandList* cmdList, int blurCount, FrameResource* frameRes);

		void _onResize(unsigned int width, unsigned int height);
		
		void _buildResources();
		void _buildOffsetVectors();
		void _buildRandomVectorTex(ID3D12GraphicsCommandList* cmdList);
		void _buildDescriptors(ID3D12Resource* depthStencilBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
			CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
			unsigned int cbvSrvUavDescriptorSize, unsigned int rtvDescriptorSize);

		ID3D12Device* m_device;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_SSAOTexture0;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_SSAOTexture1;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_normalDepthTexture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_randomVectorTex;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_randomVectorTexUpload;


		CD3DX12_CPU_DESCRIPTOR_HANDLE m_SSAOTex0CPUSrv;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_SSAOTex0CPURtv;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_SSAOTex0GPUSrv;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_SSAOTex1CPUSrv;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_SSAOTex1CPURtv;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_SSAOTex1GPUSrv;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_normalDepthCpuSrv;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_normalDepthGPUSrv;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_normalDepthCpuRtv;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_depthTexCpuSrv;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_depthTexGpuSrv;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_randomVectorCpuSrv;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_randomVectorGPUSrv;

		ID3D12PipelineState* m_SSAOPipelineState;
		ID3D12PipelineState* m_BlurPipelineState;
		

		D3D12_VIEWPORT m_viewPort;
		D3D12_RECT m_scissorRect;

		UINT m_width;
		UINT m_height;

		DirectX::XMFLOAT4	m_offsets[14];

	};
}

