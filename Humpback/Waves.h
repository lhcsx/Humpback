// Li Hongcheng
// 2022-10-30


#pragma once

#include <vector>
#include <DirectXMath.h>


namespace Humpback
{
	class Waves
	{
	public:
		Waves(int m, int n, float dx, float dt, float speed, float damping);
		Waves(const Waves& rhs) = delete;
		Waves& operator=(const Waves& rhs) = delete;
		~Waves();

		int RowCount() const;
		int ColumnCount() const;
		int TriangleCount() const;
		float Width() const;
		float Depth() const;

		const DirectX::XMFLOAT3& Position(int i) const { return m_currSolution[i]; }
		const DirectX::XMFLOAT3& Normal(int i) const { return m_normals[i]; }
		const DirectX::XMFLOAT3& TangentX(int i) const { return m_tangentX[i]; }

		void Update(float dt);
		void Disturb(int i, int j, float magnitude);

	private:
		int m_numRows = 0;
		int m_numCols = 0;

		int m_vertexCount = 0;
		int m_triangleCount = 0;

		float mK1 = .0f;
		float mK2 = .0f;
		float mK3 = .0f;

		float m_timeStep = .0f;
		float m_spatialStep = .0f;

		std::vector<DirectX::XMFLOAT3> m_prevSolution;
		std::vector<DirectX::XMFLOAT3> m_currSolution;
		std::vector<DirectX::XMFLOAT3> m_normals;
		std::vector<DirectX::XMFLOAT3> m_tangentX;
	};
}