#pragma once

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

		Vector4(float x, float y, float z, float w)
		{
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
		}

		Vector4(const Vector4& vector) = default;
		Vector4& operator=(const Vector4& vector) = default;
		~Vector4() = default;

		Vector4& operator+(const Vector4& vector);
	};

	Vector4 Vector4::operator+(const Vector4& vector) 
	{
		Vector4 v;
		v.x = this->x + v.x;
		v.y = this->y + v.y;
		v.z = this->z + v.z;
		v.w = this->w + v.w;
		return v;
	}
}
