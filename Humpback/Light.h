// (c) Li Hongcheng
// 2021-10-28


#pragma once


#include <DirectXMath.h>


namespace Humpback
{

#define MaxLights 16


	struct Light
	{
		DirectX::XMFLOAT3 strength = { 0.5f, 0.5f, 0.5f };
		float falloffStart = 1.0f;
		DirectX::XMFLOAT3 direction = { 0.0f, -1.0f, 0.0f };
		float falloffEnd = 10.0f;
		DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
		float spotPower = 64.0f;
	};
}
