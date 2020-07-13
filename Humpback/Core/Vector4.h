#pragma once

#include<math.h>

namespace Humpback 
{
	class Vector4 
	{
	public:
		float x;
		float y;
		float z;
		float w;

		Vector4()
		{
			x = .0f;
			y = .0f;
			z = .0f;
			w = .0f;
		}

		Vector4(float x, float y, float z, float w = .0f)
		{
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
		}

		Vector4(const Vector4& vector) = default;
		Vector4& operator=(const Vector4& vector) = default;
		~Vector4() = default;

		Vector4 operator+(const Vector4& vector);
		Vector4 operator-(const Vector4& vector);
		Vector4 operator*(float f);
		Vector4 operator/(float f);

		float Magnitude();
		Vector4 Normalized();
		float Dot(const Vector4&);
	};

	inline Vector4 Vector4::operator+(const Vector4& vector) 
	{
		Vector4 v;
		v.x = this->x + vector.x;
		v.y = this->y + vector.y;
		v.z = this->z + vector.z;
		v.w = this->w + vector.w;
		return v;
	}

	inline Vector4 Vector4::operator-(const Vector4& vector)
	{
		Vector4 v;
		v.x = this->x - vector.x;
		v.y = this->y - vector.y;
		v.z = this->z - vector.z;
		v.w = this->w - vector.w;
		return v;
	}

	inline Vector4 Vector4::operator*(float f)
	{
		Vector4 v;
		v.x = this->x * f;
		v.y = this->y * f;
		v.z = this->z * f;
		v.w = this->w * f;
		return v;
	}

	inline Vector4 Vector4::operator/(float f)
	{
		if (f == 0)
		{
			return Vector4(.0f, .0f, .0f, .0f);
		}

		Vector4 v;
		v.x = this->x / f;
		v.y = this->y / f;
		v.z = this->z / f;
		v.w = this->w / f;
		return v;
	}

	inline float Vector4::Magnitude()
	{
		return sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
	}

	inline Vector4 Vector4::Normalized()
	{
		Vector4 v;
		float magnitude = this->Magnitude();
		v.x = this->x / magnitude;
		v.y = this->y / magnitude;
		v.z = this->z / magnitude;
		return v;
	}

	inline float Vector4::Dot(const Vector4& vector)
	{
		return this->x * vector.x + this->y * vector.y + this->z * vector.z + this->w * vector.w;
	}

	inline Vector4 operator-(const Vector4& vector)
	{
		Vector4 v;
		v.x = -vector.x;
		v.y = -vector.y;
		v.z = -vector.z;
		v.w = -vector.w;
		return v;
	}

	
}
