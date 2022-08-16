#pragma once

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace Rose
{

	struct VKMemAllocations
	{
		uint32_t BufferAllocs = 0;
		uint32_t ImageAllocs = 0;
	};

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

			void Free(VmaAllocation allocation, VkBuffer& buffer);
			void Free(VmaAllocation allocation, VkImage image);


			static VmaAllocator& GetVMAAllocator();


		private :
			static VKMemAllocations s_Allocations;
			static VmaAllocator s_Allocator;
	};

}