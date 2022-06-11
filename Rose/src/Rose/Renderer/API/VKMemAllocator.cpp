#include "VKMemAllocator.h"

#include "Rose/Core/Application.h"
#include "Rose/Core/LOG.h"




namespace Rose
{

	uint32_t VKMemAllocator::s_Allocations=0;

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
		LOG("Called VMA shutdown...there are '%d' allocations allocated\n", s_Allocations);
		vmaDestroyAllocator(s_Allocator);
	}

	VmaAllocation VKMemAllocator::AllocateBuffer(VkBufferCreateInfo createInfo, VmaMemoryUsage usage, VkBuffer* outBuffer)
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
		s_Allocations++;
		LOG("VMA AllocBuffer: %d\n", s_Allocations);
		return result;

	}

	VmaAllocation VKMemAllocator::AllocateImage(VkImageCreateInfo createInfo, VmaMemoryUsage usage, VkImage* outImage)
	{
		if (!outImage)
		{
			LOG("Out image is null!\n");
			ASSERT();
		}


		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = usage;

		VmaAllocation result;
		vmaCreateImage(s_Allocator, &createInfo, &allocCreateInfo, outImage, &result, nullptr);

		s_Allocations++;
		LOG("VMA AllocImage: %d\n", s_Allocations);
		return result;

	}


	void VKMemAllocator::Map(VmaAllocation allocation, void** data)
	{
		vmaMapMemory(s_Allocator, allocation, &(*data));
	}

	void VKMemAllocator::UnMap(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_Allocator, allocation);
	}

	void VKMemAllocator::Free(VmaAllocation allocation, VkBuffer buffer)
	{
		vmaDestroyBuffer(s_Allocator, buffer, allocation);
		s_Allocations--;
		LOG("VMA FreeBuffer: %d\n", s_Allocations);
	}


	void VKMemAllocator::Free(VmaAllocation allocation, VkImage image)
	{
		vmaDestroyImage(s_Allocator, image, allocation);
		s_Allocations--;
		LOG("VMA FreeImage: %d\n", s_Allocations);
	}


	VmaAllocator& VKMemAllocator::GetVMAAllocator()
	{
		return s_Allocator;
	}


}