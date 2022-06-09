#pragma once

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace Rose
{

	class VKMemAllocator
	{

		public :
			VKMemAllocator() = default;

			static void Init();
			static void Shutdown();

			VmaAllocation AllocateBuffer(VkBufferCreateInfo createInfo, VmaMemoryUsage usage, VkBuffer* outBuffer);
			VmaAllocation AllocateImage(VkImageCreateInfo createInfo, VmaMemoryUsage usage, VkImage* outImage);

			void Map(VmaAllocation allocation, void** data);
			void UnMap(VmaAllocation allocation);

			void Free(VmaAllocation allocation, VkBuffer buffer);
			void Free(VmaAllocation allocation, VkImage image);


			static VmaAllocator& GetVMAAllocator();


		private :
			static VmaAllocator s_Allocator;
	};

}