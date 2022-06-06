#include "RenderDevice.h"
#include "RendererContext.h"

#include "Rose/Core/Log.h"

#include <vector>

namespace Rose
{
	//////////////////////////////////////////////////////////////////////////
	/////////////////////// PhysicalRenderingDevice	//////////////////////////
	//////////////////////////////////////////////////////////////////////////

	PhysicalRenderingDevice::PhysicalRenderingDevice()
	{
		auto& vkInstance = RendererContext::GetInstance();


		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);

		std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, devices.data());

		VkPhysicalDevice selectedGPUDevice = nullptr;
		for (auto& device : devices)
		{
			vkGetPhysicalDeviceProperties(device, &m_PhysicalDeviceProps);
			if (m_PhysicalDeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				selectedGPUDevice = device;
				break;
			}

		}

		if (!selectedGPUDevice)
		{
			LOG("Could not find a discrete GPU, falling back to an intergrated one!\n");
			selectedGPUDevice = devices.back();
		}

		m_PhysicalDevice = selectedGPUDevice;
		
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalMemProps);





		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());


		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				LOG("This GPU supports %d extensions :\n", extensions.size());
				for (const auto& ext : extensions)
				{
					LOG("%s\n", ext.extensionName);
				}
			}
		}


		int queueTypesToQuery = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;

		int i = 0;
		int found = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueTypesToQuery & VK_QUEUE_GRAPHICS_BIT)
			{
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					m_QueueFamilyIndicies.Graphics = i;
					found = 1;
				}
			}

			if (queueTypesToQuery & VK_QUEUE_TRANSFER_BIT)
			{
				if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && found == 1)
				{
					m_QueueFamilyIndicies.Transfer = i;
					found = 2;
				}
			}

			if (found == 2)
				break;
			i++;
		}




		float queuePriority = 1.0f;
		if (queueTypesToQuery & VK_QUEUE_GRAPHICS_BIT)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = m_QueueFamilyIndicies.Graphics;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
		}

		if (queueTypesToQuery & VK_QUEUE_TRANSFER_BIT)
		{
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_QueueFamilyIndicies.Transfer;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &queuePriority;

		}

	


	}

	PhysicalRenderingDevice::~PhysicalRenderingDevice()
	{

	}

}