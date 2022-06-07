#pragma once


#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include "VKMemAllocator.h"


namespace Rose
{

	class VertexBuffer
	{

		public:
	
			VertexBuffer(void* data, uint32_t size);
			VertexBuffer(uint32_t size);
	
			~VertexBuffer();
	
			void Bind();
			void SetData(void* data, uint32_t size);

			void FreeMemory();

			const VkBuffer& GetBufferID() const { return m_BufferID; }
	
		private :
			void RecreateBuffer();
		private:
			VkBuffer m_BufferID;
			VmaAllocation m_MemoryAllocaton;
	
			uint32_t m_Size = 0;
			bool m_IsFreed = false;
	
	};

}