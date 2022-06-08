#include "Texture.h"

#include "stb_image/stb_image.h"


namespace Rose
{



	Texture2D::Texture2D(const std::string& filepath)
	{

		
		uint8_t* textureBuffer = stbi_load(filepath.c_str(), &m_Width, &m_Height, &m_BPP, 4);


		stbi_image_free(textureBuffer);
	}

	Texture2D::~Texture2D()
	{

	}

}