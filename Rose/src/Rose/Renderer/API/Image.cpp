#include "Image.h"


#include "Rose/Core/Log.h"
#include "Rose/Core/Application.h"

namespace Rose
{


	Image::Image(int width, int height, VkSampleCountFlagBits samples, bool isNormalMap, uint32_t mipMapLevel, bool isHDRI, bool isCubemap)
	{
		m_IsCubemap = isCubemap;
		m_IsFreed = false;
		m_MipLevel = mipMapLevel;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		imageInfo.imageType = VK_IMAGE_TYPE_2D;

		imageInfo.extent.width = static_cast<uint32_t>(width);
		imageInfo.extent.height = static_cast<uint32_t>(height);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = m_MipLevel;
		if (m_IsCubemap)
		{
			imageInfo.arrayLayers = 6;
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}
		else
			imageInfo.arrayLayers = 1;


		if(!isNormalMap)
			imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		else
			imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

		if (isHDRI)
			imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

		if (isCubemap)
		{
			imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
		}

		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		if(!isCubemap)
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if(!isCubemap)
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		else
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = samples;



		VKMemAllocator allocator;
		m_MemoryAllocation = allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, &m_BufferID);

	}

	Image::~Image()
	{
		if (!m_IsFreed)
			Destroy();

	}

	void Image::TransitionLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, bool cube)
	{
		
		auto& device = Application::Get().GetContext()->GetLogicalDevice();
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


		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		if (!cube)
			barrier.oldLayout = oldLayout;
		else
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_BufferID;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = m_MipLevel;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;


		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

			if (cube)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			} 
			else
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			ASSERT();
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);


		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->GetQueue());

		vkFreeCommandBuffers(device->GetDevice(), device->GetCommandPool(), 1, &commandBuffer);


	}

	void Image::CopyBufferToImage(VkBuffer srcBuffer, int width, int height)
	{

		auto& device = Application::Get().GetContext()->GetLogicalDevice();
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


		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		if(m_IsCubemap)
			region.imageSubresource.layerCount = 6;
		else
			region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { (uint32_t)width, (uint32_t)height, 1};



		vkCmdCopyBufferToImage(commandBuffer, srcBuffer, m_BufferID, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->GetQueue());

		vkFreeCommandBuffers(device->GetDevice(), device->GetCommandPool(), 1, &commandBuffer);

	}

	void Image::CreateImageViews(VkFormat format, uint32_t amount)
	{

		if (!amount)
		{
			LOG("No amount was specified for an image view!\n");
			ASSERT();
		}


		for (int i = 0; i < amount; i++)
		{

			VkImageView imageView;

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_BufferID;
			if(m_IsCubemap)
				viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			else
				viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = m_MipLevel;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			if(m_IsCubemap)
				viewInfo.subresourceRange.layerCount = 6;
			else
				viewInfo.subresourceRange.layerCount = 1;


			vkCreateImageView(Application::Get().GetContext()->GetLogicalDevice()->GetDevice(), &viewInfo, nullptr, &imageView);

			m_ImageViews.push_back(imageView);
		}

	}

	void Image::Destroy()
	{
		m_IsFreed = true;

		for (auto& view : m_ImageViews)
		{
			vkDestroyImageView(Application::Get().GetContext()->GetLogicalDevice()->GetDevice(), view, nullptr);
		}

		VKMemAllocator allocator;
		allocator.Free(m_MemoryAllocation, m_BufferID);

	}

}