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


			void Destroy();

		private :
			VkImage m_BufferID;
			VmaAllocation m_MemoryAllocation;

			bool m_IsFreed = true;
	};

}