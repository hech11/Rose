#include "Texture.h"


#include "Rose/Core/Log.h"
#include "stb_image/stb_image.h"

#include "Rose/Core/Application.h"
#include "Rose/Core/Skybox.h"


namespace Rose
{



	Texture2D::Texture2D(const std::string& filepath, const TextureProperties& props)
		: m_Props(props)
	{

		//stbi_set_flip_vertically_on_load(1);
		m_IsFreed = false;
		uint8_t* textureBuffer = nullptr;
		uint32_t size = 0;
		bool isHDRI = false;

		if (stbi_is_hdr(filepath.c_str()))
		{
			textureBuffer = (uint8_t*)stbi_loadf(filepath.c_str(), &m_Width, &m_Height, &m_BPP, 4);
			size = m_Width * m_Height * 4*sizeof(float);
			isHDRI = true;
		}
		else
		{
			textureBuffer = stbi_load(filepath.c_str(), &m_Width, &m_Height, &m_BPP, 4);
			size = m_Width * m_Height * 4;
		}

		m_MipLevel = (uint32_t)std::floor(std::log2(std::max(m_Width, m_Height))) + 1;


		if (!textureBuffer)
		{
			LOG("Failed to load '%s'!\n", filepath.c_str());
			return;
		}

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
		if(isHDRI)
			imgFormat = VK_FORMAT_R32G32B32A32_SFLOAT;



		m_Image = std::make_shared<Image>(m_Width, m_Height, VK_SAMPLE_COUNT_1_BIT, m_Props.IsNormalMap, m_MipLevel, isHDRI);


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

		//createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		createInfo.compareOp = VK_COMPARE_OP_NEVER;
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



	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////// TextureCube //////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	TextureCube::TextureCube(const TextureCubeFiles& filepaths)
	{
		m_IsFreed = false;
		uint8_t* textureBuffer = nullptr;
		uint32_t size = 0;
		bool isHDRI = false;

		std::vector<uint8_t*> textureBuffers;
		std::vector<std::string> paths;
		paths.push_back(filepaths.PosX);
		paths.push_back(filepaths.NegX);
		paths.push_back(filepaths.PosY);
		paths.push_back(filepaths.NegY);
		paths.push_back(filepaths.PosZ);
		paths.push_back(filepaths.NegZ);

		for (int i = 0; i < paths.size(); i++)
		{
			stbi_set_flip_vertically_on_load(false);

			if (stbi_is_hdr(paths[i].c_str()))
			{
				uint8_t* temp = (uint8_t*)stbi_loadf(paths[i].c_str(), &m_Width, &m_Height, &m_BPP, 4);
				textureBuffers.push_back(temp);
				isHDRI = true;
			}
			else
			{
				uint8_t* temp = stbi_load(paths[i].c_str(), &m_Width, &m_Height, &m_BPP, 4);
				textureBuffers.push_back(temp);
			}
		}
		//stbi_set_flip_vertically_on_load(true);

		if(isHDRI)
			size = m_Width * m_Height * 4 * 6*sizeof(float);
		else
			size = m_Width * m_Height * 4 * 6;


		m_MipLevel = (uint32_t)std::floor(std::log2(std::max(m_Width, m_Height))) + 1;


	

		
		auto& device = Application::Get().GetContext()->GetLogicalDevice();


		uint32_t layerSize = size / 6;
		VkFormat imgFormat = VK_FORMAT_R8G8B8A8_UNORM;
		if (isHDRI)
			imgFormat = VK_FORMAT_R32G32B32A32_SFLOAT;




		VKMemAllocator allocator;
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = imgFormat;
		imageCreateInfo.mipLevels = m_MipLevel;
		imageCreateInfo.arrayLayers = 6;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.extent = { (uint32_t)m_Width, (uint32_t)m_Height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		m_ImageMemoryAllocation = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, &m_Image);

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkBuffer stagingBuffer;
		VmaAllocation  tempAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, &stagingBuffer);


		uint8_t* temp;
		vmaMapMemory(VKMemAllocator::GetVMAAllocator(), tempAllocation, (void**)&temp);
		for(int i = 0; i < textureBuffers.size(); i++)
		{
			memcpy(temp + (layerSize * i), textureBuffers[i], layerSize);
		}
		
		vmaUnmapMemory(VKMemAllocator::GetVMAAllocator(), tempAllocation);


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

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 6;

		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = m_Image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 6;
		bufferCopyRegion.imageExtent.width = m_Width;
		bufferCopyRegion.imageExtent.height = m_Height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		vkCmdCopyBufferToImage(
			commandBuffer,
			stagingBuffer,
			m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion);

		VkImageMemoryBarrier imageMemoryBarrier2{};
		imageMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		imageMemoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier2.image = m_Image;
		imageMemoryBarrier2.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier2);
		
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->GetQueue());

		vkFreeCommandBuffers(device->GetDevice(), device->GetCommandPool(), 1, &commandBuffer);

		for (auto& buffer : textureBuffers)
		{
			stbi_image_free(buffer);
		}
		allocator.Free(tempAllocation, stagingBuffer);



		VkCommandBufferAllocateInfo allocInfo2{};
		allocInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo2.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo2.commandPool = device->GetCommandPool();
		allocInfo2.commandBufferCount = 1;

		VkCommandBuffer layoutCommand;
		vkAllocateCommandBuffers(device->GetDevice(), &allocInfo2, &layoutCommand);

		VkCommandBufferBeginInfo beginInfo2{};
		beginInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo2.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(layoutCommand, &beginInfo2);

		VkImageSubresourceRange subresourceRange2 = {};
		subresourceRange2.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange2.baseMipLevel = 0;
		subresourceRange2.levelCount = m_MipLevel;
		subresourceRange2.layerCount = 6;


		VkImageMemoryBarrier imageMemoryBarrier3 = {};
		imageMemoryBarrier3.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier3.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier3.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier3.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier3.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier3.image = m_Image;
		imageMemoryBarrier3.subresourceRange = subresourceRange2;
		imageMemoryBarrier3.srcAccessMask = 0;

		vkCmdPipelineBarrier(
			layoutCommand,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier3);




		CreateSampler();


		VkImageViewCreateInfo view{};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = imgFormat;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 6;
		view.subresourceRange.levelCount = m_MipLevel;
		view.image = m_Image;
		vkCreateImageView(Application::Get().GetContext()->GetLogicalDevice()->GetDevice(), &view, nullptr, &m_ImageView);


		GenerateMips(imgFormat);

	}

	TextureCube::~TextureCube()
	{
		if (!m_IsFreed)
		{
			Destroy();
		}
	}

	void TextureCube::Destroy()
	{
		m_IsFreed = true;
		auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		vkDestroySampler(device, m_Sampler, nullptr);
	}

	void TextureCube::CreateSampler()
	{
		auto& physicalDevice = Application::Get().GetContext()->GetPhysicalDevice()->GetDevice();
		auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		VkPhysicalDeviceProperties deviceProps{};
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		createInfo.compareOp = VK_COMPARE_OP_NEVER;
		createInfo.compareEnable = VK_FALSE;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = (float)m_MipLevel;

		VkResult result = vkCreateSampler(device, &createInfo, nullptr, &m_Sampler);
	}

	void TextureCube::GenerateMips(VkFormat imageFormat)
	{
		auto& device = Application::Get().GetContext()->GetLogicalDevice();
		auto& physicalDevice = Application::Get().GetContext()->GetPhysicalDevice();

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice->GetDevice(), imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			ASSERT();
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



		uint32_t mipLevels = m_MipLevel;
		for (uint32_t face = 0; face < 6; face++)
		{
			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = 0;
			mipSubRange.baseArrayLayer = face;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;


			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.image = m_Image;
			imageMemoryBarrier.subresourceRange = mipSubRange;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			for (uint32_t face = 0; face < 6; face++)
			{
				VkImageBlit imageBlit{};

				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = face;
				imageBlit.srcOffsets[1].x = int32_t(m_Width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(m_Height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = face;
				imageBlit.dstOffsets[1].x = int32_t(m_Width >> i);
				imageBlit.dstOffsets[1].y = int32_t(m_Height >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.baseArrayLayer = face;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.image = m_Image;
				imageMemoryBarrier.subresourceRange = mipSubRange;

				vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);

				vkCmdBlitImage(
					commandBuffer,
					m_Image,
					VK_IMAGE_LAYOUT_GENERAL,
					m_Image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&imageBlit,
					VK_FILTER_LINEAR);

			


				VkImageMemoryBarrier imageMemoryBarrier2{};
				imageMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageMemoryBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				imageMemoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier2.image = m_Image;
				imageMemoryBarrier2.subresourceRange = mipSubRange;

				vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier2);
			}
		}

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 6;
		subresourceRange.levelCount = mipLevels;


		VkImageMemoryBarrier imageMemoryBarrier3{};
		imageMemoryBarrier3.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier3.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier3.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		imageMemoryBarrier3.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier3.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier3.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier3.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier3.image = m_Image;
		imageMemoryBarrier3.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier3);


		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->GetQueue());

		vkFreeCommandBuffers(device->GetDevice(), device->GetCommandPool(), 1, &commandBuffer);


	}







	std::shared_ptr<TextureCube> EnviormentTexture::GetIrradianceMap()
	{
#if USESKY2

		TextureCubeFiles files =
		{
			"assets/textures/skybox/sky2/irr/output_iem_posx.tga",
			"assets/textures/skybox/sky2/irr/output_iem_negx.tga",
			"assets/textures/skybox/sky2/irr/output_iem_posy.tga",
			"assets/textures/skybox/sky2/irr/output_iem_negy.tga",
			"assets/textures/skybox/sky2/irr/output_iem_posz.tga",
			"assets/textures/skybox/sky2/irr/output_iem_negz.tga",
 		};
#endif
#if USESKY1
		TextureCubeFiles files =
		{
			"assets/textures/skybox/sky1/irr/output_iem_posx.tga",
			"assets/textures/skybox/sky1/irr/output_iem_negx.tga",
			"assets/textures/skybox/sky1/irr/output_iem_posy.tga",
			"assets/textures/skybox/sky1/irr/output_iem_negy.tga",
			"assets/textures/skybox/sky1/irr/output_iem_posz.tga",
			"assets/textures/skybox/sky1/irr/output_iem_negz.tga",
		};
#endif

		static std::shared_ptr<TextureCube> Instance = std::make_shared<TextureCube>(files);
		return Instance;
	}

	std::shared_ptr<TextureCube> EnviormentTexture::GetRadienceMap()
	{
#if USESKY2

 		TextureCubeFiles files =
 		{
 			"assets/textures/skybox/sky2/rad/output_pmrem_posx_0_256x256.tga",
 			"assets/textures/skybox/sky2/rad/output_pmrem_negx_0_256x256.tga",
 			"assets/textures/skybox/sky2/rad/output_pmrem_posy_0_256x256.tga",
 			"assets/textures/skybox/sky2/rad/output_pmrem_negy_0_256x256.tga",
 			"assets/textures/skybox/sky2/rad/output_pmrem_posz_0_256x256.tga",
 			"assets/textures/skybox/sky2/rad/output_pmrem_negz_0_256x256.tga",
 		};
#endif
#if USESKY1
		TextureCubeFiles files =
		{
			"assets/textures/skybox/sky1/rad/output_pmrem_posx_0_256x256.tga",
			"assets/textures/skybox/sky1/rad/output_pmrem_negx_0_256x256.tga",
			"assets/textures/skybox/sky1/rad/output_pmrem_posy_0_256x256.tga",
			"assets/textures/skybox/sky1/rad/output_pmrem_negy_0_256x256.tga",
			"assets/textures/skybox/sky1/rad/output_pmrem_posz_0_256x256.tga",
			"assets/textures/skybox/sky1/rad/output_pmrem_negz_0_256x256.tga",
		};
#endif
		static std::shared_ptr<TextureCube> Instance = std::make_shared<TextureCube>(files);
		return Instance;
	}

	std::shared_ptr<Texture2D> EnviormentTexture::GetSpecularBRDF()
	{
		TextureProperties props = { false };
		static std::shared_ptr<Texture2D> Instance = std::make_shared<Texture2D>("assets/textures/BRDF_LUT.tga", props);
		return Instance;
	}

	
}