#include "Model.h"


#include "Rose/Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <filesystem>


namespace Rose
{

	Model::Model(const std::string& filepath) 
		: m_Filepath(filepath)
	{

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filepath, aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_CalcTangentSpace);


		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOG("ASSIMP Failed!\tError: %s\n", importer.GetErrorString());
			ASSERT();
			return;
		}

		ProcessNode(scene->mRootNode, scene);
	}


	void Model::CleanUp()
	{
		for (auto& material : m_Materials)
		{
			material.ShaderData->DestroyPipeline();
			for (auto& uniform : material.Uniforms)
			{
				uniform.Texture->Destroy();
			}
		}
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
			Vertex vertex;

			vertex.Position.x = mesh->mVertices[i].x;
			vertex.Position.y = mesh->mVertices[i].y;
			vertex.Position.z = mesh->mVertices[i].z;

 			vertex.Normal.x = mesh->mNormals[i].x;
 			vertex.Normal.y = mesh->mNormals[i].y;
 			vertex.Normal.z = mesh->mNormals[i].z;
 

			vertex.Tangent.x = mesh->mTangents[i].x;
			vertex.Tangent.y = mesh->mTangents[i].y;
			vertex.Tangent.z = mesh->mTangents[i].z;

			if (mesh->mTextureCoords[0])
			{
				vertex.TexCoord.x = mesh->mTextureCoords[0][i].x;
				vertex.TexCoord.y = mesh->mTextureCoords[0][i].y;
			}

			result.Verticies.push_back(vertex);
		}


		

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
			{
				result.Indicies.push_back(face.mIndices[j]);
			}
		}

		if (mesh->mMaterialIndex >= 0)
		{
			Material result;
			result.Name = "No name";


			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			auto specMaps = LoadTextures(material, aiTextureType_SPECULAR);
			result.Uniforms.insert(result.Uniforms.end(), specMaps.begin(), specMaps.end());

			auto normMaps = LoadTextures(material, aiTextureType_NORMALS);
			result.Uniforms.insert(result.Uniforms.end(), normMaps.begin(), normMaps.end());

			auto diffMaps = LoadTextures(material, aiTextureType_DIFFUSE);
			result.Uniforms.insert(result.Uniforms.end(), diffMaps.begin(), diffMaps.end());


		
			ShaderAttributeLayout layout =
			{
				{"a_Position", 0, ShaderMemberType::Float3},
				{"a_Normal", 1, ShaderMemberType::Float3},
				{"a_Tangent", 2, ShaderMemberType::Float3},
				{"a_TexCoord", 3, ShaderMemberType::Float2}
			};

			result.ShaderData = std::make_shared<Rose::Shader>("assets/shaders/main.shader", layout);
		
			result.ShaderData->CreatePipelineAndDescriptorPool(result.Uniforms);

			m_Materials.push_back(result);

		}

		

		return result;
	}

	std::vector<MaterialUniform> Model::LoadTextures(aiMaterial* mat, aiTextureType type)
	{
		std::vector<MaterialUniform> result;

		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			MaterialUniform uniform;

			if (type == aiTextureType_DIFFUSE)
				uniform.TextureType = PBRTextureType::Albedo;
			else if (type == aiTextureType_SPECULAR)
				uniform.TextureType = PBRTextureType::Specular;
			else if (type == aiTextureType_NORMALS)
				uniform.TextureType = PBRTextureType::Normal;


			aiString str;
			mat->GetTexture(type, i, &str);


			std::filesystem::path modelPath = m_Filepath;
			std::filesystem::path modelParentPath = modelPath.parent_path();
			TextureProperties props;
			props.IsNormalMap = (type == aiTextureType_NORMALS);

			uniform.Texture = std::make_shared<Texture2D>(modelParentPath.string() + std::string("/") +std::string(str.C_Str()), props);

			result.push_back(uniform);
		}
		return result;
	}

}