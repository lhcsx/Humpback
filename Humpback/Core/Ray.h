#pragma once

#include"Vector4.h"

namespace Humpback 
{
	class Ray
	{
	public:
		Ray(Vector4 origin, Vector4 direction);
		Ray(const Ray& ray) = default;
		Ray& operator=(const Ray& ray) = default;

		Vector4 GetOrigin() const { return mOrigin; };
		Vector4 GetDirection() const { return mDirection; };
		float GetT() const { return mT; }

		Vector4 PositionAtT(float t) const;

	private:
		Vector4 mOrigin;
		Vector4 mDirection;
		float mT;
	};
}