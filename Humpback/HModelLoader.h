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


	class HModelLoader
	{
	public:
		HModelLoader(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList);
		~HModelLoader();

		bool Load(std::string fileName);

	private:

		void _processNode(aiNode* node, const aiScene* scene);
		Mesh _processMesh(aiMesh* pAiMesh, const aiScene* pAiScene);
		std::vector<Texture> _loadMaterialTextures(aiMaterial* pMat, aiTextureType type, std::string typeName, const aiScene* pScene);
		void _loadEmbededTexture(const aiTexture* pAiTexture, Texture* pTexture);

		std::vector<Texture> m_texturesLoaded;
		
		ComPtr<ID3D12Device>	m_device = nullptr;
		ComPtr<ID3D12GraphicsCommandList>	m_commandList = nullptr;
		
		std::vector<Mesh> m_meshes;
	};
}