// (c) Li Hongcheng
// 2023-02-26


#pragma once

#include "D3DUtil.h"


namespace Humpback
{
	class ShadowMap
	{
	public:

		ShadowMap(ID3D12Device* device, unsigned int width, unsigned int height);
		~ShadowMap() = default;

		ShadowMap(const ShadowMap& rhs) = delete;
		ShadowMap& operator=(const ShadowMap& rhs) = delete;

		unsigned int Width() const;
		unsigned int Height() const;

		D3D12_VIEWPORT GetViewPort() const;
		D3D12_RECT GetScissorRect() const;

		CD3DX12_GPU_DESCRIPTOR_HANDLE SRV() const;
		CD3DX12_CPU_DESCRIPTOR_HANDLE DSV() const;

		ID3D12Resource* Resource();

		void BuildDescriptors(
			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSRVHandle,
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSRVHandle,
			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDSVHandle
		);

		void OnResize(unsigned int newWidth, unsigned int newHeight);

	private:

		void _buildResource();
		void _buildDescriptors();

		ID3D12Device* m_device = nullptr;

		unsigned int m_width = 0;
		unsigned int m_height = 0;

		D3D12_VIEWPORT m_viewPort;
		D3D12_RECT m_scissorRect;

		DXGI_FORMAT m_format = DXGI_FORMAT_R24G8_TYPELESS;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_cpuSRVHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_gpuSRVHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_cpuDSVHandle;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_shadowMap = nullptr;
	};
}