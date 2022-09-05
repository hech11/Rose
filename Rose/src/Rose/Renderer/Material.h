#pragma once

#include "API/Texture.h"
#include "API/Shader.h"
#include <vector>

namespace Rose
{


	enum class PBRTextureType
	{
		Albedo, Specular, Normal, Irr, SpecBRDF, Rad
	};

	struct MaterialUniform // We may not need this as we could put uniforms into its own UBO instead unless its a sampler.
	{
		std::shared_ptr<Texture2D> Texture;
		std::shared_ptr<TextureCube> Texture3DCube;
		PBRTextureType TextureType;
	};
	struct Material
	{
		std::string Name;
		std::vector<MaterialUniform> Uniforms;
		std::shared_ptr<Rose::Shader> ShaderData;

		~Material()
		{
			//ShaderData->DestroyPipeline();
		}


		static std::shared_ptr<Texture2D> DefaultWhiteTexture()
		{
			static std::shared_ptr<Texture2D> white = std::make_shared<Texture2D>("assets/textures/white.png");
			return white;
		}
	};



}