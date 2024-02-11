// (c) Li Hongcheng
// 2023-12-31

#include <iterator>
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "HModelLoader.h"
#include "Vertex.h"
#include "D3DUtil.h"
#include "DDSTextureLoader.h"

#pragma comment(lib, "Assimp/lib/x64/assimp-vc143-mt.lib")


namespace Humpback
{
	void ThrowIfFailed(HRESULT hr);
	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter);
	std::wstring GetAssetPath(std::wstring str);

	HModelLoader::HModelLoader(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList):
		m_device(pDevice), m_commandList(pCmdList)
	{
	}

	HModelLoader::~HModelLoader()
	{
	}

	bool HModelLoader::Load(std::string fileName)
	{
		Assimp::Importer importer;

		const aiScene* pScene = importer.ReadFile(fileName, 
			aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

		if (pScene == nullptr)
		{
			return false;
		}

		_processNode(pScene->mRootNode, pScene);

		return true;
	}

	Mesh* HModelLoader::GetMesh()
	{
		return &m_meshes[0];
	}

	void HModelLoader::_processNode(aiNode* node, const aiScene* scene)
	{
		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* pAiMesh = scene->mMeshes[node->mMeshes[i]];
			m_meshes.push_back(_processMesh(pAiMesh, scene));
		}

		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			_processNode(node->mChildren[i], scene);
		}
	}

	Mesh HModelLoader::_processMesh(aiMesh* pAiMesh, const aiScene* pAiScene)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		// Parse vertices.
		for (size_t i = 0; i < pAiMesh->mNumVertices; i++)
		{
			// Position.
			Vertex v;
			v.position.x = pAiMesh->mVertices[i].x;
			v.position.y = pAiMesh->mVertices[i].y;
			v.position.z = pAiMesh->mVertices[i].z;

			// UV 0
			if (pAiMesh->mTextureCoords[0])
			{
				v.uv.x = pAiMesh->mTextureCoords[0][i].x;
				v.uv.y = pAiMesh->mTextureCoords[0][i].y;
			}

			// Normal
			v.normal.x = pAiMesh->mNormals[i].x;
			v.normal.y = pAiMesh->mNormals[i].y;
			v.normal.z = pAiMesh->mNormals[i].z;

			// Tangent
			v.tangent.x = pAiMesh->mTangents[i].x;
			v.tangent.y = pAiMesh->mTangents[i].y;
			v.tangent.z = pAiMesh->mTangents[i].z;

			vertices.push_back(v);
		}

		// Parse indices.
		for (size_t i = 0; i < pAiMesh->mNumFaces; i++)
		{
			aiFace face = pAiMesh->mFaces[i];
			for (size_t j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		// Parse materials.
		if (pAiMesh->mMaterialIndex >= 0)
		{
			aiMaterial* pMaterial = pAiScene->mMaterials[pAiMesh->mMaterialIndex];
			std::vector<Texture> diffuseMaps = this->_loadMaterialTextures(pMaterial, aiTextureType_DIFFUSE, "texture_diffuse", pAiScene);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		}


		Mesh mesh;

		const unsigned int vbByteSize = (unsigned int)vertices.size() * sizeof(Vertex);
		const unsigned int ibByteSize = (unsigned int)indices.size() * sizeof(unsigned int);

		// Vertex data.
		ThrowIfFailed(D3DCreateBlob(vbByteSize, &mesh.vertexBufferCPU));
		CopyMemory(mesh.vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		// Indices data.
		ThrowIfFailed(D3DCreateBlob(ibByteSize, &mesh.indexBufferCPU));
		CopyMemory(mesh.indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		mesh.vertexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			vertices.data(), vbByteSize, mesh.vertexBufferUploader);

		mesh.indexBufferGPU = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
			indices.data(), ibByteSize, mesh.indexBufferUploader);

		mesh.vertexByteStride = sizeof(Vertex);
		mesh.vertexBufferByteSize = vbByteSize;
		mesh.indexFormat = DXGI_FORMAT_R32_UINT;
		mesh.indexBufferByteSize = ibByteSize;

		SubMesh mainMesh;
		mainMesh.indexCount = indices.size();
		mainMesh.startIndexLocation = 0;
		mainMesh.baseVertexLocation = 0;

		mesh.drawArgs["main"] = mainMesh;

		return mesh;
	}
	std::vector<Texture> HModelLoader::_loadMaterialTextures(aiMaterial* pMat, aiTextureType texType, std::string typeName, const aiScene* pScene)
	{
		std::vector<Texture> textures;
		for (size_t i = 0; i < pMat->GetTextureCount(texType); i++)
		{
			aiString str;
			pMat->GetTexture(texType, i, &str);
			
			std::wstring wstr;
			std::copy(str.C_Str(), str.C_Str() + strlen(str.C_Str()), std::back_inserter(wstr));

			// Check if this texture is already loaded before.
			bool skip = false;
			for (size_t j = 0; j < m_texturesLoaded.size(); j++)
			{

				if (std::wcscmp(m_texturesLoaded[j].filePath.c_str(), wstr.c_str()) == 0)
				{
					textures.push_back(m_texturesLoaded[j]);
					skip = true;
					break;
				}
			}

			if (skip == false)
			{
				HRESULT hr;
				Texture texture;

				const aiTexture* embededTexture = pScene->GetEmbeddedTexture(str.C_Str());
				if (embededTexture == nullptr)
				{
					// Load from file.
					ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_device.Get(), m_commandList.Get(),
						wstr.c_str(), texture.resource, texture.uploadHeap));
				}
				else
				{
					_loadEmbededTexture(embededTexture, &texture);
				}

				texture.type = texType;
				texture.filePath = wstr;
				textures.push_back(texture);
				m_texturesLoaded.push_back(texture);
			}
		}

		return std::vector<Texture>();
	}

	void HModelLoader::_loadEmbededTexture(const aiTexture* pAiTexture, Texture* pTexture)
	{

		D3D12_SUBRESOURCE_DATA subresourceData;
		subresourceData.pData = pAiTexture->pcData;
		subresourceData.RowPitch = pAiTexture->mWidth * 4;
		subresourceData.SlicePitch = pAiTexture->mWidth * pAiTexture->mHeight * 4;

		HRESULT hr;
		hr = D3DUtil::CreateD3DResources12(
			m_device.Get(), m_commandList.Get(), D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			pAiTexture->mWidth, pAiTexture->mHeight, 1, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, false, false
			, &subresourceData, pTexture->resource, pTexture->uploadHeap);

		ThrowIfFailed(hr);
	}

	
}

