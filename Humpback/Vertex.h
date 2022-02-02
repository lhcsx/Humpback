// (c) Li Hongcheng
// 2022/02/02

#pragma once

#include <DirectXMath.h>


struct Vertex
{
	Vertex() :m_position(), m_normal() {}

	Vertex(float px, float py, float pz, float nx, float ny, float nz,
		float tx, float ty, float tz, float u, float v) :
		m_position(px, py, pz), m_normal(nx, ny, nz), m_tangentU(tx, ty, tz), m_uv(u, v) {}

	Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, 
		const DirectX::XMFLOAT3& tangentU, const DirectX::XMFLOAT2& uv) :
		m_position(position), m_normal(normal), m_tangentU(tangentU), m_uv(uv) {}



	DirectX::XMFLOAT3 m_position;
	DirectX::XMFLOAT3 m_normal;
	DirectX::XMFLOAT3 m_tangentU;
	DirectX::XMFLOAT2 m_uv;
};