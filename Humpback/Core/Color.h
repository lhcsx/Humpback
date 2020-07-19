#pragma once

#include<math.h>
#include"Vector4.h"

namespace Humpback
{
	class Color
	{
	public:
		float r;
		float g;
		float b;
		float a;

		Color()
		{
			r = .0f;
			g = .0f;
			b = .0f;
			a = 1.0f;
		}

		Color(float r, float g, float b, float a = 1.0f)
		{
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}

		Color(const Color&) = default;
		Color& operator=(const Color&) = default;
		~Color() = default;

		Color operator+(const Color&);
		Color operator-(const Color&);
		Color operator/(float);
		Color operator*(float);
		Color operator*(const Color&);
		Color operator*(const Vector4&);
	};

	inline Color Color::operator+(const Color& color)
	{
		Color v;
		v.r = this->r + color.r;
		v.g = this->g + color.g;
		v.b = this->b + color.b;
		v.a = this->a + color.a;
		return v;
	}

	inline Color Color::operator-(const Color& color)
	{
		Color v;
		v.r = this->r - color.r;
		v.g = this->g - color.g;
		v.b = this->b - color.b;
		v.a = this->a - color.a;
		return v;
	}

	inline Color Color::operator*(float f)
	{
		Color v;
		v.r = this->r * f;
		v.g = this->g * f;
		v.b = this->b * f;
		v.a = this->a * f;
		return v;
	}

	inline Color Color::operator*(const Color& c)
	{
		Color color;
		color.r = this->r * c.r;
		color.g = this->g * c.g;
		color.b = this->b * c.b;
		color.a = this->a * c.a;
		return color;
	}

	inline Color Color::operator*(const Vector4& v)
	{
		Color color;
		color.r = this->r * v.x;
		color.g = this->g * v.y;
		color.b = this->b * v.z;
		color.a = this->a * v.w;
		return color;
	}

	inline Color Color::operator/(float f)
	{
		if (f == 0)
		{
			return Color(.0f, .0f, .0f, .0f);
		}

		Color v;
		v.r = this->r / f;
		v.g = this->g / f;
		v.b = this->b / f;
		v.a = this->a / f;
		return v;
	}
}
