// (c) Li Hongcheng
// 2022/03/19


#include "Mesh.h"


namespace Humpback
{
	Mesh::Mesh() : m_name(""), m_vertices(nullptr), m_indices(nullptr)
	{
		m_vertices = new std::vector<Vertex>();
		m_indices = new std::vector<uint32_t>();
	}
}