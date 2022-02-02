// (c) Li Hongcheng
// 2022/01/26

#pragma once

#include <string>
#include <vector>

#include "Vertex.h"


namespace Humpback 
{

	struct Mesh
	{
	public:

		std::string GetName() { return m_name; }
		std::vector<Vertex> GetVertices() { return m_vertices; }
		std::vector<uint32_t> GetIndices() { return m_indices; }
		void SetVertices(std::vector<Vertex> vertices) { m_vertices = vertices; }
		void SetIndices(std::vector<uint32_t> indices) { m_indices = indices; }


	private:

		std::string m_name;
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};

}

