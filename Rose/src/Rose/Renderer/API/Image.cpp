#include "Image.h"


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

	void Image::Destroy()
	{
		m_IsFreed = true;

		VKMemAllocator allocator;
		allocator.Free(m_MemoryAllocation, m_BufferID);

	}

}