#include "VKMemAllocator.h"

#include "Rose/Core/Application.h"
#include "Rose/Core/LOG.h"




namespace Rose
{

	VmaAllocator VKMemAllocator::s_Allocator;



	void VKMemAllocator::Init()
	{
		VmaAllocatorCreateInfo createInfo{};
		createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		createInfo.physicalDevice = Application::Get().GetContext()->GetPhysicalDevice()->GetDevice();
		createInfo.device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();
		createInfo.instance = Application::Get().GetContext()->GetInstance();

		vmaCreateAllocator(&createInfo, &s_Allocator);

	}

	void VKMemAllocator::Shutdown()
	{
		vmaDestroyAllocator(s_Allocator);
	}

	VmaAllocation VKMemAllocator::Allocate(VkBufferCreateInfo createInfo, VmaMemoryUsage usage, VkBuffer* outBuffer)
	{
		if (!outBuffer)
		{
			LOG("Out buffer is null!\n");
			ASSERT();
		}


		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = usage;

		VmaAllocation result;
		vmaCreateBuffer(s_Allocator, &createInfo, &allocCreateInfo, outBuffer, &result, nullptr);

		return result;

	}

	void VKMemAllocator::Free(VmaAllocation allocation, VkBuffer buffer)
	{
		vmaDestroyBuffer(s_Allocator, buffer, allocation);
	}

	VmaAllocator& VKMemAllocator::GetVMAAllocator()
	{
		return s_Allocator;
	}


}