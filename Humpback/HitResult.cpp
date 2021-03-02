#include "HitResult.h"


namespace Humpback
{
	HitResult::HitResult(): nearestHitPoint(Vector4::PZero()), farestHitPoint(Vector4::PZero()), 
		t0(0.0f), t1(0.0f), pObject(nullptr)
	{

	}
	HitResult::~HitResult()
	{
	}
}



