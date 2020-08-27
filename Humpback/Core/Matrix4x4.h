#pragma once

#include<Vector4.h>

namespace Humpback
{
	class Matrix4x4
	{
	public:
		static Matrix4x4 Identity();

		Matrix4x4();

		Matrix4x4(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3);
		Matrix4x4(float m00, float m01, float m02, float m03,
			      float m10, float m11, float m12, float m13,
			      float m20, float m21, float m22, float m23,
			      float m30, float m31, float m32, float m33);


		 float* operator[](int);
		 bool operator==(Matrix4x4&) const;
		 Matrix4x4 operator*(Matrix4x4&) const;
		 Vector4 operator*(const Vector4&) const;

		 Matrix4x4 Transpose() const;
		 float Determinant() const;
		 float MINOR(const size_t r0, const size_t r1, const size_t r2,
			 const size_t c0, const size_t c1, const size_t c2) const;

		 Matrix4x4 Inverse() const;


	private:
		float m[4][4] = {.0f};
	};
}