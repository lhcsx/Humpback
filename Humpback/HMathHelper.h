// (c) Li Hongcheng
// 2022-05-04


#pragma once

#include <stdlib.h>
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

		static float RandF()
		{
			return (float)(rand()) / (float)RAND_MAX;
		}

		static float RandF(float a, float b)
		{
			return a + RandF() * (b - a);
		}

		static int Rand(int a, int b)
		{
			return a + rand() % ((b - a) + 1);
		}




		static const float PI;
	};
}