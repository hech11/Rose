#include "Model.h"


#include "Rose/Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
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
	}

}