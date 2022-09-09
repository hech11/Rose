#pragma once

#include <string>
#include <memory>


#include "Image.h"

namespace Rose
{


	// TODO: Expand on this!
	struct TextureProperties
	{
		bool IsNormalMap = false;
	};

	struct TextureCubeFiles
	{
		std::string PosX;
		std::string NegX;
		std::string PosY;
		std::string NegY;
		std::string PosZ;
		std::string NegZ;
	};

	class Texture2D
	{

		public :

			Texture2D(const std::string& filepath, const TextureProperties& props = TextureProperties());
			~Texture2D();

			const VkImageView& GetImageView() const { return m_Image->GetImageViews()[0]; }
			const VkSampler& GetSampler() const { return  m_Sampler; }

			void Destroy();

		private :
			void CreateSampler();
			void GenerateMips(VkFormat imageFormat);


		private :
			int32_t m_Width, m_Height, m_BPP;
			uint32_t m_MipLevel;

			TextureProperties m_Props;

			VkSampler m_Sampler;
			std::shared_ptr<Image> m_Image;
			bool m_IsFreed = true;

	};


	class TextureCube
	{
		public :
			TextureCube(const TextureCubeFiles& filepaths);
			~TextureCube();

			const VkImageView& GetImageView() const { return m_ImageView; }
			const VkSampler& GetSampler() const { return  m_Sampler; }

			void Destroy();

		private:
			void CreateSampler();
			void GenerateMips(VkFormat imageFormat);
		
		
		private:
			int32_t m_Width, m_Height, m_BPP;
			uint32_t m_MipLevel;
		
		
			VkSampler m_Sampler;
			VkImage m_Image;
			VkImageView m_ImageView;
			VmaAllocation m_ImageMemoryAllocation;

			bool m_IsFreed = true;
	};


	class EnviormentTexture
	{
		public :
			static std::shared_ptr<TextureCube> GetIrradianceMap();
			static std::shared_ptr<TextureCube> GetRadienceMap();
			static std::shared_ptr<Texture2D> GetSpecularBRDF();
			
	};

}