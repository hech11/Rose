#pragma once

#include "Mesh.h"

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

		private :
			void ProcessNode(aiNode* node, const aiScene* scene);
			Mesh CreateMesh(aiMesh* mesh, const aiScene* scene);

		private :
			std::vector<Mesh> m_Meshes;
	};

}