#include "Model.h"


#include "Rose/Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>


namespace Rose
{

	Model::Model(const std::string& filepath)
	{

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filepath, aiPostProcessSteps::aiProcess_Triangulate);


		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOG("ASSIMP Failed!\tError: %s\n", importer.GetErrorString());
			ASSERT();
			return;
		}

		ProcessNode(scene->mRootNode, scene);
	}


	void Model::ProcessNode(aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			m_Meshes.push_back(CreateMesh(mesh, scene));
		}
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene);
		}


	}

	Rose::Mesh Model::CreateMesh(aiMesh* mesh, const aiScene* scene)
	{

		Mesh result;

		int mult3 = 0;
		Triangle triangle{};

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			if (mult3 == 3)
			{
				mult3 = 0;
				result.Triangles.push_back(triangle);
			}


			Vertex vertex;

			vertex.Position.x = mesh->mVertices[i].x;
			vertex.Position.y = mesh->mVertices[i].y;
			vertex.Position.z = mesh->mVertices[i].z;

 			vertex.Normal.x = mesh->mNormals[i].x;
 			vertex.Normal.y = mesh->mNormals[i].y;
 			vertex.Normal.z = mesh->mNormals[i].z;
 
 			vertex.TexCoord.x = 0.0f;
 			vertex.TexCoord.y = 0.0f;

			triangle.Verticies[mult3] = vertex;

			mult3++;
		}


		if (mult3 == 3)
		{
			Vertex vertex;
			int i = mesh->mNumVertices;
			vertex.Position.x = mesh->mVertices[i].x;
			vertex.Position.y = mesh->mVertices[i].y;
			vertex.Position.z = mesh->mVertices[i].z;

			vertex.Normal.x = mesh->mNormals[i].x;
			vertex.Normal.y = mesh->mNormals[i].y;
			vertex.Normal.z = mesh->mNormals[i].z;

			if (mesh->mTextureCoords[0])
			{
				vertex.TexCoord.x = mesh->mTextureCoords[0][i].x;
				vertex.TexCoord.y = mesh->mTextureCoords[0][i].y;
			}

			result.Triangles.push_back(triangle);

		}

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
			{
				result.Indicies.push_back(face.mIndices[j]);
			}
		}

		return result;
	}

}