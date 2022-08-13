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


}