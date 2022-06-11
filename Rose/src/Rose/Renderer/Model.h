#pragma once

#include "Mesh.h"
#include "Material.h"

#include <string>

#include <assimp/scene.h>

namespace Rose

{

	class Model
	{

		public :
			Model(const std::string& filepath);


			const std::vector<Mesh>& GetMeshes() const { return m_Meshes; }
			std::vector<Mesh>& GetMeshes() { return m_Meshes; }

			const std::vector<Material>& GetMaterials() const { return m_Materials; }
			std::vector<Material>& GetMaterials() { return m_Materials; }

		private :
			void ProcessNode(aiNode* node, const aiScene* scene);
			Mesh CreateMesh(aiMesh* mesh, const aiScene* scene);
			std::vector<MaterialUniform> LoadTextures(aiMaterial* mat, aiTextureType type);

		private :

			std::string m_Filepath;
			std::vector<Mesh> m_Meshes;
			std::vector<Material> m_Materials;
	};

}