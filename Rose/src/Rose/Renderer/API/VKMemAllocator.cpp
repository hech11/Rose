#include "VKMemAllocator.h"

#include "Rose/Core/Application.h"
#include "Rose/Core/LOG.h"




namespace Rose
{

	VKMemAllocations  VKMemAllocator::s_Allocations{};
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
		if (s_Allocations.BufferAllocs)
		{
			LOG("Called VMA shutdown...there are '%d' buffer allocations allocated\n", s_Allocations.BufferAllocs);
		}

		if (s_Allocations.ImageAllocs)
		{
			LOG("Called VMA shutdown...there are '%d' image allocations allocated\n", s_Allocations.ImageAllocs);
		}

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
		s_Allocations.BufferAllocs++;
		LOG("VMA AllocBuffer [%p] | total allocations: %d\n", (void*)outBuffer, s_Allocations.BufferAllocs);
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

		s_Allocations.ImageAllocs++;
		LOG("VMA AllocImage: %d\n", s_Allocations.ImageAllocs);
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

	void VKMemAllocator::Free(VmaAllocation allocation, VkBuffer& buffer)
	{
		vmaDestroyBuffer(s_Allocator, buffer, allocation);

		s_Allocations.BufferAllocs--;
		LOG("VMA FreeBuffer [%p] | total allocations: %d\n", (void*)(&buffer), s_Allocations.BufferAllocs);
	}


	void VKMemAllocator::Free(VmaAllocation allocation, VkImage image)
	{
		vmaDestroyImage(s_Allocator, image, allocation);
		s_Allocations.ImageAllocs--;
		LOG("VMA FreeImage: %d\n", s_Allocations.ImageAllocs);
	}


	VmaAllocator& VKMemAllocator::GetVMAAllocator()
	{
		return s_Allocator;
	}


}