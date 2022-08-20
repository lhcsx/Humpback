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

		template<typename T>
		static T Clamp(const T& x, const T& low, const T& high)
		{
			return x < low ? low : (x > high ? high : x);
		}

		static const float PI;
	};
}