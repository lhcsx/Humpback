#pragma once

#include <vector>

#include "Vector4.h"

namespace Humpback
{
	class Sphere;


	class HitResult
	{
	public:
		HitResult();
		~HitResult();

		Vector4 nearestHitPoint;
		Vector4 farestHitPoint;

		float t0;
		float t1;

		Sphere* pObject;
	};
}
