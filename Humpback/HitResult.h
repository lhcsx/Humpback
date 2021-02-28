#pragma once

#include <vector>

#include "Vector4.h"

namespace Humpback
{
	class HitResult
	{
	public:
		Vector4 nearestHitPoint;
		Vector4 farestHitPoint;
	};
}
