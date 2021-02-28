#pragma once

#include "Vector4.h"
#include "Ray.h"
#include "HitResult.h"

namespace Humpback
{
	class Sphere
	{
	public:
		Sphere();
		Sphere(const Vector4& origin, float radius);

		bool Intersect(const Ray& ray, HitResult& hitResult);



	private:
		Vector4 mOrigin;
		float mRadius;
	};
}
