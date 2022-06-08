#include "Texture.h"


#include "Rose/Core/Log.h"
#include "stb_image/stb_image.h"
#include "Image.h"



namespace Rose
{



	Texture2D::Texture2D(const std::string& filepath)
	{

		
		uint8_t* textureBuffer = stbi_load(filepath.c_str(), &m_Width, &m_Height, &m_BPP, 4);
		if (!textureBuffer)
		{
			LOG("Failed to load '%s'!\n", filepath.c_str());
			return;
		}
		uint32_t size = m_Width * m_Height * 4;

		VKMemAllocator allocator;

		VkBuffer tempBuffer;
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocation tempAllocation =  allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, &tempBuffer);


		m_Image = std::make_shared<Image>(m_Width, m_Height);

		m_Image->TransitionLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		m_Image->CopyBufferToImage(tempBuffer, m_Width, m_Height);
		m_Image->TransitionLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		allocator.Free(tempAllocation, tempBuffer);
		stbi_image_free(textureBuffer);
	}

	Texture2D::~Texture2D()
	{
		m_Image->Destroy();
	}

}