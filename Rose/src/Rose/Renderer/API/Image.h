#pragma once

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include "VKMemAllocator.h"

#include <vector>

namespace Rose
{



	class Image
	{

		public :
			Image(int width, int height, VkSampleCountFlagBits samples, bool isNormalMap = false, uint32_t mipMapLevel = 1);
			~Image();

			const VkImage& GetImageBuffer() const { return m_BufferID; }
			VkImage& GetImageBuffer() { return m_BufferID; }

			void TransitionLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
			void CopyBufferToImage(VkBuffer srcBuffer, int width, int height);

			void CreateImageViews(VkFormat format, uint32_t amount = 1); // TODO: mulitple views could have different formats?

			void Destroy();

			const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }


		private :
			VkImage m_BufferID;
			VmaAllocation m_MemoryAllocation;
			uint32_t m_MipLevel = 0;
			std::vector<VkImageView> m_ImageViews;

			bool m_IsFreed = true;
	};

}