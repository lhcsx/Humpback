// (c) Li Hongcheng
// 2022-10-02


#pragma once

#include <vector>

#include <DirectXMath.h>



namespace Humpback
{
	
	class GeometryGenerator
	{
	public:

		using uint16 = std::uint16_t;
		using uint32 = std::uint32_t;

		struct Vertex
		{
			Vertex() = default;
			Vertex(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& n,
				const DirectX::XMFLOAT3& t, const DirectX::XMFLOAT2& uv)
				:Position(p), Normal(n), TangentU(t), TexC(uv)
			{

			}

			Vertex(
				float px, float py, float pz,
				float nx, float ny, float nz,
				float tx, float ty, float tz,
				float u, float v) :
				Position(px, py, pz),
				Normal(nx, ny, nz),
				TangentU(tx, ty, tz),
				TexC(u, v) {}


			DirectX::XMFLOAT3 Position;
			DirectX::XMFLOAT3 Normal;
			DirectX::XMFLOAT3 TangentU;
			DirectX::XMFLOAT2 TexC;
		};

		struct MeshData
		{
			std::vector<Vertex> vertices;
			std::vector<uint32> indices32;

			std::vector<uint16>& GetIndices16()
			{
				if (m_indices16.empty())
				{
					m_indices16.resize(indices32.size());
					for (size_t i = 0; i < indices32.size(); i++)
					{
						m_indices16[i] = static_cast<uint16>(indices32[i]);
					}
				}

				return m_indices16;
			}

		private:
			std::vector<uint16> m_indices16;
		};


		MeshData CreateBox(float w, float h, float d, uint32 numSubdivisions);
		MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);
		MeshData CreateGeosphere(float radius, uint32 numSubdivisions);
		MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);
		MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);
		MeshData CreateQuad(float x, float y, float w, float h, float depth);

	private:
		void _subdivide(MeshData& meshData);
		Vertex _midPoint(const Vertex& v0, const Vertex& v1);
		void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
		void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
	};
}
