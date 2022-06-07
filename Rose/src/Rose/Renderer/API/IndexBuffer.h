#pragma once

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include "VKMemAllocator.h"

namespace Rose
{
	class IndexBuffer
	{

		public:
	
			IndexBuffer(void* data, uint32_t size);
			IndexBuffer(uint32_t size);
	
			~IndexBuffer();
	
			void Bind();
			void SetData(void* data, uint32_t size);
	
			void FreeMemory();
	
			const VkBuffer& GetBufferID() const { return m_BufferID; }
	
		private:
			void RecreateBuffer();
		private:
			VkBuffer m_BufferID;
			VmaAllocation m_MemoryAllocation;
	
			uint32_t m_Size = 0;
			bool m_IsFreed = false;

	};

}


