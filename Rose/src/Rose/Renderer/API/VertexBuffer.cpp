#include "VertexBuffer.h"
#include "Rose/Core/Application.h"


namespace Rose
{

	VertexBuffer::VertexBuffer(void* data, uint32_t size )
		: m_Size(size)
	{
		RecreateBuffer();
		Bind();
		SetData(data, size);
	}

	VertexBuffer::VertexBuffer(uint32_t size) 
		: m_Size(size)
	{
		
		RecreateBuffer();
	}

	VertexBuffer::~VertexBuffer()
	{
		if(!m_IsFreed)
			FreeMemory();
	}

	void VertexBuffer::Bind()
	{
		const VkDevice& logicalDeivce = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();
		vkBindBufferMemory(logicalDeivce, m_BufferID , m_DeviceMemory, 0);
	}

	void VertexBuffer::SetData(void* data, uint32_t size)
	{

		m_Size = size;
		const VkDevice& logicalDeivce = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();


		void* temp = nullptr;
		vkMapMemory(logicalDeivce, m_DeviceMemory, 0, size, 0, &temp);
		memcpy(temp, data, size);
		vkUnmapMemory(logicalDeivce, m_DeviceMemory);

	}

	void VertexBuffer::FreeMemory()
	{
		m_IsFreed = true;
		const VkDevice& logicalDeivce = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();
		vkDestroyBuffer(logicalDeivce, m_BufferID, nullptr);
		vkFreeMemory(logicalDeivce, m_DeviceMemory, nullptr);
	}

	void VertexBuffer::RecreateBuffer()
	{
		m_IsFreed = false;


		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_Size;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		const VkDevice& logicalDeivce = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();
		const VkPhysicalDevice& physicalDevice = Application::Get().GetContext()->GetPhysicalDevice()->GetDevice();


		vkCreateBuffer(logicalDeivce, &bufferInfo, nullptr, &m_BufferID);


		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDeivce, m_BufferID, &memRequirements);


		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		uint32_t memTypeIndex = 0;
		uint32_t propFilter = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & propFilter) == propFilter)
			{
				memTypeIndex = i;
				break;
			}
		}

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memTypeIndex;

		vkAllocateMemory(logicalDeivce, &allocInfo, nullptr, &m_DeviceMemory);
	}

}