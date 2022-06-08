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

		private :
			void CreateSampler();

		private :
			int32_t m_Width, m_Height, m_BPP;


			VkSampler m_Sampler;
			std::shared_ptr<Image> m_Image;

	};


}