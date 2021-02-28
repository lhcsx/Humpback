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

		static Vector4 VZero();
		static Vector4 PZero();

		Vector4 operator+(const Vector4& vector) const;
		Vector4 operator-(const Vector4& vector) const;
		Vector4 operator*(float f) const;
		Vector4 operator/(float f) const;

		float Magnitude() const;
		Vector4 Normalized() const;
		float Dot(const Vector4&) const;
		Vector4 Cross(const Vector4&) const;
	};
	// ------------------------------------------------------------------------------------------
	inline Vector4 Vector4::VZero()
	{
		return Vector4(.0f, .0f, .0f, .0f);
	}
	// ------------------------------------------------------------------------------------------
	inline Vector4 Vector4::PZero()
	{
		return Vector4(.0f, .0f, .0f, 1.f);
	}
	// ------------------------------------------------------------------------------------------
	inline Vector4 Vector4::operator+(const Vector4& vector) const
	{
		Vector4 v;
		v.x = this->x + vector.x;
		v.y = this->y + vector.y;
		v.z = this->z + vector.z;
		v.w = this->w + vector.w;
		return v;
	}
	// ------------------------------------------------------------------------------------------
	inline Vector4 Vector4::operator-(const Vector4& vector) const
	{
		Vector4 v;
		v.x = this->x - vector.x;
		v.y = this->y - vector.y;
		v.z = this->z - vector.z;
		v.w = this->w - vector.w;
		return v;
	}
	// ------------------------------------------------------------------------------------------
	inline Vector4 Vector4::operator*(float f) const
	{
		Vector4 v;
		v.x = this->x * f;
		v.y = this->y * f;
		v.z = this->z * f;
		v.w = this->w * f;
		return v;
	}
	// ------------------------------------------------------------------------------------------
	inline Vector4 Vector4::operator/(float f) const
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
	// ------------------------------------------------------------------------------------------
	inline float Vector4::Magnitude() const
	{
		double x = this->x;
		double y = this->y;
		double z = this->z;

		return static_cast<float>(sqrt(x * x + y * y + z * z));
	}
	// ------------------------------------------------------------------------------------------
	inline Vector4 Vector4::Normalized() const
	{
		Vector4 v;
		float magnitude = this->Magnitude();
		v.x = this->x / magnitude;
		v.y = this->y / magnitude;
		v.z = this->z / magnitude;
		return v;
	}
	// ------------------------------------------------------------------------------------------
	inline float Vector4::Dot(const Vector4& vector) const
	{
		return this->x * vector.x + this->y * vector.y + this->z * vector.z + this->w * vector.w;
	}
	// ------------------------------------------------------------------------------------------
	// Only considering the x, y, and z component.
	inline Vector4 Vector4::Cross(const Vector4& v) const
	{
		return Vector4(this->y * v.z - this->z * v.y, 
			this->z * v.x - this->x * v.z, this->x * v.y - this->y * v.x);
	}
	// ------------------------------------------------------------------------------------------
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
