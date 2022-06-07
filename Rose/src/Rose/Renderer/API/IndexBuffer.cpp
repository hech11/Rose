#include "IndexBuffer.h"
#include "Rose/Core/Application.h"


namespace Rose
{

	IndexBuffer::IndexBuffer(void* data, uint32_t size) 
		: m_Size(size)
	{
		RecreateBuffer();
		Bind();
		SetData(data, size);
	}

	IndexBuffer::IndexBuffer(uint32_t size)
		: m_Size(size)
	{
		RecreateBuffer();
	}

	IndexBuffer::~IndexBuffer()
	{
		if (!m_IsFreed)
			FreeMemory();
	}

	void IndexBuffer::Bind()
	{

		vmaBindBufferMemory(VKMemAllocator::GetVMAAllocator(), m_MemoryAllocation, m_BufferID);
	}

	void IndexBuffer::SetData(void* data, uint32_t size)
	{
		m_Size = size;

		void* temp = nullptr;
		vmaMapMemory(VKMemAllocator::GetVMAAllocator(), m_MemoryAllocation, &temp);
		memcpy(temp, data, size);
		vmaUnmapMemory(VKMemAllocator::GetVMAAllocator(), m_MemoryAllocation);
	}

	void IndexBuffer::FreeMemory()
	{
		m_IsFreed = true;
		VKMemAllocator allocator;
		allocator.Free(m_MemoryAllocation, m_BufferID);
	}

	void IndexBuffer::RecreateBuffer()
	{
		m_IsFreed = false;
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_Size;
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		VKMemAllocator allocator;
		m_MemoryAllocation = allocator.Allocate(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, &m_BufferID);

	}

}