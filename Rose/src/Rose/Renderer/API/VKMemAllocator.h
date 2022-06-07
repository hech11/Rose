#pragma once

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace Rose
{

	class VKMemAllocator
	{

		public :
			VKMemAllocator() = default;

			void Init();
			void Shutdown();

			VmaAllocation Allocate(VkBufferCreateInfo createInfo, VmaMemoryUsage usage, VkBuffer* outBuffer);
			void Free(VmaAllocation allocation, VkBuffer buffer);


			static VmaAllocator& GetVMAAllocator();


		private :
			static VmaAllocator s_Allocator;
	};

}