#pragma once

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include "VKMemAllocator.h"

namespace Rose
{


	class Image
	{

		public :
			Image(int width, int height);
			~Image();

			const VkImage& GetImageBuffer() const { return m_BufferID; }
			VkImage& GetImageBuffer() { return m_BufferID; }

			void TransitionLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
			void CopyBufferToImage(VkBuffer srcBuffer, int width, int height);


			void Destroy();

		private :
			VkImage m_BufferID;
			VmaAllocation m_MemoryAllocation;

			bool m_IsFreed = true;
	};

}