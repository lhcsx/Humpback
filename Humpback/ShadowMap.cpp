// (c) Li Hongcheng
// 2023-02-26


#include "ShadowMap.h"


namespace Humpback
{
    void ThrowIfFailed(HRESULT hr);

    ShadowMap::ShadowMap(ID3D12Device* device, unsigned int width, unsigned int height)
    {
        m_width = width;
        m_height = height;

        m_device = device;

        m_viewPort = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
        m_scissorRect = { 0, 0, (int)width, (int)height };

        _buildResource();
    }

    unsigned int ShadowMap::Width() const
    {
        return m_width;
    }

    unsigned int ShadowMap::Height() const
    {
        return m_height;
    }

    D3D12_VIEWPORT ShadowMap::GetViewPort() const
    {
        return m_viewPort;
    }

    D3D12_RECT ShadowMap::GetScissorRect() const
    {
        return m_scissorRect;
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::SRV() const
    {
        return m_gpuSRVHandle;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::DSV() const
    {
        return m_cpuDSVHandle;
    }

    ID3D12Resource* ShadowMap::Resource()
    {
        return m_shadowMap.Get();
    }

    void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSRVHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSRVHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDSVHandle)
    {
        m_cpuSRVHandle = cpuSRVHandle;
        m_gpuSRVHandle = gpuSRVHandle;
        m_cpuDSVHandle = cpuDSVHandle;

        _buildDescriptors();
    }

    void ShadowMap::OnResize(unsigned int newWidth, unsigned int newHeight)
    {
        if ((newWidth != m_width) || (newHeight != m_height))
        {
            m_width = newWidth;
            m_height = newHeight;

            _buildResource();
            _buildDescriptors();
        }
    }

    void ShadowMap::_buildResource()
    {
        D3D12_RESOURCE_DESC smDesc;
        ZeroMemory(&smDesc, sizeof(smDesc));

        smDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        smDesc.Format = m_format;
        smDesc.MipLevels = 1;
        smDesc.Alignment = 0;
        smDesc.DepthOrArraySize = 1;
        smDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        smDesc.Width = m_width;
        smDesc.Height = m_height;
        smDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        smDesc.SampleDesc.Count = 1;
        smDesc.SampleDesc.Quality = 0;

        D3D12_CLEAR_VALUE clearValue;
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,&smDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue, IID_PPV_ARGS(&m_shadowMap)));
    }

    void ShadowMap::_buildDescriptors()
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Texture2D.PlaneSlice = 0;
        m_device->CreateShaderResourceView(m_shadowMap.Get(), &srvDesc, m_cpuSRVHandle);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.Texture2D.MipSlice = 0;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        m_device->CreateDepthStencilView(m_shadowMap.Get(), &dsvDesc, m_cpuDSVHandle);
    }
}