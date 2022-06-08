#include "Texture.h"


#include "Rose/Core/Log.h"
#include "stb_image/stb_image.h"

#include "Rose/Core/Application.h"


namespace Rose
{



	Texture2D::Texture2D(const std::string& filepath)
	{

		stbi_set_flip_vertically_on_load(1);
		m_IsFreed = false;
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


		void* temp;
		allocator.Map(tempAllocation, &temp);
		memcpy(temp, textureBuffer, size);
		allocator.UnMap(tempAllocation);


		m_Image = std::make_shared<Image>(m_Width, m_Height);

		m_Image->TransitionLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		m_Image->CopyBufferToImage(tempBuffer, m_Width, m_Height);
		m_Image->TransitionLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		allocator.Free(tempAllocation, tempBuffer);
		stbi_image_free(textureBuffer);


		m_Image->CreateImageViews(VK_FORMAT_R8G8B8A8_SRGB);
		CreateSampler();


	}

	Texture2D::~Texture2D()
	{
		if (!m_IsFreed)
		{
			Destroy();
		}
		
	}

	void Texture2D::Destroy()
	{
		m_IsFreed = true;
		auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		vkDestroySampler(device, m_Sampler, nullptr);
		m_Image->Destroy();
	}

	void Texture2D::CreateSampler()
	{

		
		auto& physicalDevice = Application::Get().GetContext()->GetPhysicalDevice()->GetDevice();
		auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		VkPhysicalDeviceProperties deviceProps{};
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

		//TODO: Make these properties dynamic
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.anisotropyEnable = VK_TRUE;
		//createInfo.maxAnisotropy = 1.0f;
		createInfo.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.compareEnable = VK_FALSE;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.0f;

		VkResult result = vkCreateSampler(device, &createInfo, nullptr, &m_Sampler);


	}

}