// (c) Li Hongcheng
// 2023-12-31

#pragma once

#include <vector>
#include <string>

#include "assimp/scene.h"

#include "Mesh.h"
#include "Texture.h"


namespace Humpback
{
	using Microsoft::WRL::ComPtr;


	class HMeshImporter
	{
	public:
		HMeshImporter(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList);
		~HMeshImporter();

		bool Load(std::string fileName);
		
		Mesh* GetMesh(int index = 0);

	private:

		void _processNode(aiNode* node, const aiScene* scene);
		Mesh _processMesh(aiMesh* pAiMesh, const aiScene* pAiScene);
		
		ComPtr<ID3D12Device>	m_device = nullptr;
		ComPtr<ID3D12GraphicsCommandList>	m_commandList = nullptr;
		
		std::vector<Mesh> m_meshes;
	};
}