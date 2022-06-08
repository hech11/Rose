#pragma once

#include <string>
#include <memory>


#include "Image.h"

namespace Rose
{

	class Texture2D
	{

		public :

			Texture2D(const std::string& filepath);
			~Texture2D();

			const VkImageView& GetImageView() const { return m_Image->GetImageViews()[0]; }
			const VkSampler& GetSampler() const { return  m_Sampler; }

			void Destroy();

		private :
			void CreateSampler();

		private :
			int32_t m_Width, m_Height, m_BPP;


			VkSampler m_Sampler;
			std::shared_ptr<Image> m_Image;
			bool m_IsFreed = true;

	};


}