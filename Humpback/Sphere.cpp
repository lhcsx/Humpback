#include "Sphere.h"


namespace Humpback
{
	Sphere::Sphere()
	{
		mOrigin = Vector4::PZero();
		mRadius = 1.0f;
	}
	// ------------------------------------------------------------------------------------------
	Sphere::Sphere(const Vector4& origin, float radius): mOrigin(origin), mRadius(radius)
	{
	}
	// ------------------------------------------------------------------------------------------
	bool Sphere::Intersect(const Ray& ray, HitResult& hitResult)
	{
		float t0, t1;
		Vector4 o = ray.GetOrigin();
		Vector4 oc = o - mOrigin;
		Vector4 d = ray.GetDirection();
		float dotOCD = d.Dot(oc);

		float delta = dotOCD * dotOCD - (oc.Dot(oc) - mRadius * mRadius);

		if (delta <= .0f)
		{
			return false;
		}

		delta = sqrt(delta);
		t0 = -dotOCD - delta;
		t1 = -dotOCD + delta;

		hitResult.nearestHitPoint = ray.PositionAtT(t0);
		hitResult.farestHitPoint = ray.PositionAtT(t1);
		hitResult.t0 = t0;
		hitResult.t1 = t1;
		hitResult.pObject = this;

		return true;
	}
}