#include "Image.h"


#include "Rose/Core/Log.h"
#include "Rose/Core/Application.h"

namespace Rose
{


	Image::Image(int width, int height)
	{
		m_IsFreed = false;
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(width);
		imageInfo.extent.height = static_cast<uint32_t>(height);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;

		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;


		VKMemAllocator allocator;
		allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, &m_BufferID);

		vmaBindImageMemory(allocator.GetVMAAllocator(), m_MemoryAllocation, m_BufferID);
	}

	Image::~Image()
	{
		if (!m_IsFreed)
			Destroy();

	}

	void Image::TransitionLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		// TODO: Create Command buffers??


		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_BufferID;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		VkCommandBuffer temp = nullptr;
		vkCmdPipelineBarrier(temp, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		// end command buffer here

	}

	void Image::CopyBufferToImage(VkBuffer srcBuffer, int width, int height)
	{

		// TODO: Create Command buffers??

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};



		VkCommandBuffer temp;
		vkCmdCopyBufferToImage(temp, srcBuffer, m_BufferID, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


		// end command buffer here

	}

	void Image::CreateImageViews(VkFormat format, uint32_t amount)
	{

		if (!amount)
		{
			LOG("No amount was specified for an image view!\n");
			ASSERT();
		}

		m_ImageViews.resize(amount);
		for (int i = 0; i < m_ImageViews.size(); i++)
		{

			VkImageView imageView;

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_BufferID;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(Application::Get().GetContext()->GetLogicalDevice()->GetDevice(), &viewInfo, nullptr, &imageView);

			m_ImageViews.emplace_back(imageView);
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