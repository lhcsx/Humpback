#include"Ray.h"


namespace Humpback 
{
	Ray::Ray(Vector4 origin, Vector4 direction): mOrigin(origin), mDirection(direction)
	{
	}
	// ------------------------------------------------------------------------------------------
	Vector4 Ray::PositionAtT(float t) const
	{
		Vector4 pos(.0f, .0f, .0f, 1.f);
		pos = mOrigin + mDirection * t;

		return pos;
	}
}