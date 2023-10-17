// (c) Li Hongcheng
// 2022-05-04

#pragma once

#include <array>
#include <xstring>
#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"

namespace Humpback 
{
    class D3DUtil
    {
    public:
        static unsigned int CalConstantBufferByteSize(unsigned int byteSize)
        {
            // Constant buffers must be a multiple of the minimum hardware
            // allocation size (usually 256 bytes).  So round up to nearest
            // multiple of 256.  We do this by adding 255 and then masking off
            // the lower 2 bytes which store all bits < 256.
            // Example: Suppose byteSize = 300.
            // (300 + 255) & ~255
            // 555 & ~255
            // 0x022B & ~0x00ff
            // 0x022B & 0xff00
            // 0x0200
            // 512
            return (byteSize + 255) & ~255;
        }

        static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
            ID3D12Device* device, 
            ID3D12GraphicsCommandList* commandList,
            const void* initData,
            uint64_t byteSize,
            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

        static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
            const std::wstring& filename,
            const D3D_SHADER_MACRO* defines,
            const std::string& entrypoint,
            const std::string_view target);

        static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetCommonStaticSamplers();
    };

}

