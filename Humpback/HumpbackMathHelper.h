// (c) Li Hongcheng
// 2022/02/16

#pragma once

#include<DirectXMath.h>

namespace Humpback
{
	class HumpbackMathHelper
	{
	public:
		HumpbackMathHelper();
		~HumpbackMathHelper();


		static DirectX::XMFLOAT4X4 IdentityMatrixF4x4();

	};
}