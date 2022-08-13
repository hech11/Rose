#include "Texture.h"


#include "Rose/Core/Log.h"
#include "stb_image/stb_image.h"

#include "Rose/Core/Application.h"


namespace Rose
{



	Texture2D::Texture2D(const std::string& filepath, const TextureProperties& props)
		: m_Props(props)
	{

		stbi_set_flip_vertically_on_load(1);
		m_IsFreed = false;
		uint8_t* textureBuffer = stbi_load(filepath.c_str(), &m_Width, &m_Height, &m_BPP, 4);

		m_MipLevel = (uint32_t)std::floor(std::log2(std::max(m_Width, m_Height))) + 1;


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

		VkFormat imgFormat = VK_FORMAT_R8G8B8A8_SRGB;
		if(m_Props.IsNormalMap)
			imgFormat = VK_FORMAT_R8G8B8A8_UNORM;


		m_Image = std::make_shared<Image>(m_Width, m_Height, VK_SAMPLE_COUNT_1_BIT, m_Props.IsNormalMap, m_MipLevel);


		m_Image->TransitionLayout(imgFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		m_Image->CopyBufferToImage(tempBuffer, m_Width, m_Height);
		//m_Image->TransitionLayout(imgFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		allocator.Free(tempAllocation, tempBuffer);
		stbi_image_free(textureBuffer);


		m_Image->CreateImageViews(imgFormat);
		CreateSampler();
		GenerateMips(imgFormat);

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
		createInfo.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.compareEnable = VK_FALSE;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = (float)m_MipLevel;

		VkResult result = vkCreateSampler(device, &createInfo, nullptr, &m_Sampler);


	}

	void Texture2D::GenerateMips(VkFormat imageFormat)
	{

		auto& device = Application::Get().GetContext()->GetLogicalDevice();
		auto& physicalDevice = Application::Get().GetContext()->GetPhysicalDevice();

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice->GetDevice(), imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
		{
			ASSERT(); // we do not support linear blitting
		}



		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device->GetCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);


		auto& image = m_Image->GetImageBuffer();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = m_Width;
		int32_t mipHeight = m_Height;



		for (uint32_t i = 1; i < m_MipLevel; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}


		barrier.subresourceRange.baseMipLevel = m_MipLevel - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);



		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->GetQueue());

		vkFreeCommandBuffers(device->GetDevice(), device->GetCommandPool(), 1, &commandBuffer);


	}

}