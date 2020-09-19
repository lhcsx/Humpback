#include<exception>

#include "Matrix4x4.h"


namespace Humpback 
{
	Matrix4x4 Matrix4x4::Identity()
	{
		Matrix4x4 res = { 1.f, .0f, .0f, .0f,
						 .0f, 1.f, .0f, .0f,
						 .0f, .0f, 1.f, .0f,
						 .0f, .0f, .0f, 1.f };

		return res;
	}
	Matrix4x4 Matrix4x4::Translation(Vector4 v)
	{
		Matrix4x4 res = Identity();
		res[0][3] = v.x;
		res[1][3] = v.y;
		res[2][3] = v.z;

		return res;
	}
	// ------------------------------------------------------------------------------------------
	Matrix4x4::Matrix4x4()
	{
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				m[i][j] = .0f;
			}
		}
	}
	// ------------------------------------------------------------------------------------------
	Matrix4x4::Matrix4x4(const Vector4 & row0, const Vector4 & row1, const Vector4 & row2, const Vector4 & row3)
	{
		m[0][0] = row0.x; m[0][1] = row0.y; m[0][2] = row0.z; m[0][3] = row0.w;
		m[1][0] = row1.x; m[1][1] = row1.y; m[1][2] = row1.z; m[1][3] = row1.w;
		m[2][0] = row2.x; m[2][1] = row2.y; m[2][2] = row2.z; m[2][3] = row2.w;
		m[3][0] = row3.x; m[3][1] = row3.y; m[3][2] = row3.z; m[3][3] = row3.w;
	}
	// ------------------------------------------------------------------------------------------
	Matrix4x4::Matrix4x4(float m00, float m01, float m02, float m03, 
		float m10, float m11, float m12, float m13, 
		float m20, float m21, float m22, float m23, 
		float m30, float m31, float m32, float m33)
	{
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
		m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
	}
	// ------------------------------------------------------------------------------------------
	float* Matrix4x4::operator[](int index)
	{
		if (index >= 4)
		{
			throw new std::exception("Input argument out of range.");
			return nullptr;
		}
		
		return m[index];
	}
	// ------------------------------------------------------------------------------------------
	bool Matrix4x4::operator==(Matrix4x4& rhs) const
	{
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				if (m[i][j] != rhs[i][j])
				{
					return false;
				}
			}
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	Matrix4x4 Matrix4x4::operator*(Matrix4x4& rhs) const
	{
		Matrix4x4 res;
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				for (size_t k = 0; k < 4; k++)
				{
					res[i][j] += (m[i][k] * rhs[k][j]);
				}
			}
		}
		return res;
	}
	// ------------------------------------------------------------------------------------------
	Vector4 Matrix4x4::operator*(const Vector4& rhs) const
	{
		return Vector4(m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3] * rhs.w,
			m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3] * rhs.w,
			m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3] * rhs.w,
			m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3] * rhs.w);
	}
	// ------------------------------------------------------------------------------------------
	Matrix4x4 Matrix4x4::Transpose() const
	{
		Matrix4x4 res;

		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				res[j][i] = m[i][j];
			}
		}

		return res;
	}
	// ------------------------------------------------------------------------------------------
	float Matrix4x4::Determinant() const
	{
		return m[0][0] * MINOR(1, 2, 3, 1, 2, 3) -
			m[0][1] * MINOR(1, 2, 3, 0, 2, 3) +
			m[0][2] * MINOR(1, 2, 3, 0, 1, 3) -
			m[0][3] * MINOR(1, 2, 3, 0, 1, 2);
	}
	// ------------------------------------------------------------------------------------------
	float Matrix4x4::MINOR(const size_t r0, const size_t r1, const size_t r2,
		const size_t c0, const size_t c1, const size_t c2) const
	{
		return m[r0][c0] * (m[r1][c1] * m[r2][c2] - m[r2][c1] * m[r1][c2]) -
			m[r0][c1] * (m[r1][c0] * m[r2][c2] - m[r2][c0] * m[r1][c2]) +
			m[r0][c2] * (m[r1][c0] * m[r2][c1] - m[r2][c0] * m[r1][c1]);
	}
	// ------------------------------------------------------------------------------------------
	Matrix4x4 Matrix4x4::Inverse() const
	{
		float m00 = m[0][0], m01 = m[0][1], m02 = m[0][2], m03 = m[0][3];
		float m10 = m[1][0], m11 = m[1][1], m12 = m[1][2], m13 = m[1][3];
		float m20 = m[2][0], m21 = m[2][1], m22 = m[2][2], m23 = m[2][3];
		float m30 = m[3][0], m31 = m[3][1], m32 = m[3][2], m33 = m[3][3];

		float v0 = m20 * m31 - m21 * m30;
		float v1 = m20 * m32 - m22 * m30;
		float v2 = m20 * m33 - m23 * m30;
		float v3 = m21 * m32 - m22 * m31;
		float v4 = m21 * m33 - m23 * m31;
		float v5 = m22 * m33 - m23 * m32;

		float t00 = +(v5 * m11 - v4 * m12 + v3 * m13);
		float t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
		float t20 = +(v4 * m10 - v2 * m11 + v0 * m13);
		float t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

		float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

		float d00 = t00 * invDet;
		float d10 = t10 * invDet;
		float d20 = t20 * invDet;
		float d30 = t30 * invDet;

		float d01 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d11 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d21 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d31 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m10 * m31 - m11 * m30;
		v1 = m10 * m32 - m12 * m30;
		v2 = m10 * m33 - m13 * m30;
		v3 = m11 * m32 - m12 * m31;
		v4 = m11 * m33 - m13 * m31;
		v5 = m12 * m33 - m13 * m32;

		float d02 = +(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d12 = -(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d22 = +(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d32 = -(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m21 * m10 - m20 * m11;
		v1 = m22 * m10 - m20 * m12;
		v2 = m23 * m10 - m20 * m13;
		v3 = m22 * m11 - m21 * m12;
		v4 = m23 * m11 - m21 * m13;
		v5 = m23 * m12 - m22 * m13;

		float d03 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d13 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d23 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d33 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		return Matrix4x4(
			d00, d01, d02, d03,
			d10, d11, d12, d13,
			d20, d21, d22, d23,
			d30, d31, d32, d33);
	}
	// ------------------------------------------------------------------------------------------
}
