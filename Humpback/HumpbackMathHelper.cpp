// (c) Li Hongcheng
// 2022/02/16


#include "HumpbackMathHelper.h"


using namespace DirectX;

namespace Humpback 
{
	HumpbackMathHelper::HumpbackMathHelper()
	{
	}

	HumpbackMathHelper::~HumpbackMathHelper()
	{
	}

	XMFLOAT4X4 HumpbackMathHelper::IdentityMatrixF4x4()
	{
		XMFLOAT4X4 i(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);

		return i;
	}
}

