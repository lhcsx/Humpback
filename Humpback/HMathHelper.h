// (c) Li Hongcheng
// 2022-05-04


#pragma once

#include <DirectXMath.h>


namespace Humpback 
{
	class HMathHelper
	{
	public:
		static DirectX::XMFLOAT4X4 Identity4x4()
		{
			return DirectX::XMFLOAT4X4(
				1.f, .0f, .0f, .0f,
				.0f, 1.f, .0f, .0f,
				.0f, .0f, 1.f, .0f,
				.0f, .0f, .0f, 1.f
			);
		}

		static const float PI;
	};
}