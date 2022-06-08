#pragma once

#include <string>

namespace Rose
{

	class Texture2D
	{

		public :

			Texture2D(const std::string& filepath);
			~Texture2D();

		private :
			int32_t m_Width, m_Height, m_BPP;


	};


}