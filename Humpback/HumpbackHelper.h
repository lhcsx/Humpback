// (c) Li Hongcheng
// 2021/10/28

#pragma once

#include <string>
#include <Windows.h>
#include <stdexcept>
#include <cstdlib>
#include <d3d12.h>
#include <wrl.h>
#include <fstream>

#include "Vertex.h"
#include "Mesh.h"

namespace Humpback 
{

	inline std::string HrToString(HRESULT hr)
	{
		char str[64] = {};
		sprintf_s(str, "Result of 0x%08X", hr);
		return std::string(str);
	}

	class HrException : public std::runtime_error
	{
	public:
		HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr)
		{

		}

		HRESULT Error() const
		{
			return m_hr;
		}
	private:
		HRESULT m_hr;
	};


	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw HrException(hr);
		}
	}

	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
	{
		*ppAdapter = nullptr;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (UINT adapterIndex = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex, requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&adapter)));
				++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 adapterDesc;
				adapter->GetDesc1(&adapterDesc);

				// Check to see where the adapter supports DirectX 12, bu do not create actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}

			*ppAdapter = adapter.Detach();
		}
	}

	std::wstring GetAssetPath(std::wstring str)
	{
		WCHAR directoryBuffer[512] = {};
		GetCurrentDirectory(512, directoryBuffer);

		std::wstring path = std::wstring(directoryBuffer);
		path += str;

		return path;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filePath)
	{
		std::ifstream fin(filePath, std::ios::binary);

		fin.seekg(0, std::ios_base::end);
		std::ifstream::pos_type size = (int)fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

		fin.read((char*)blob->GetBufferPointer(), size);
		fin.close();

		return blob;
	}
}

