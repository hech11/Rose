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








	//////////////////////////////////////////////////////////////////////////
	/////////////////////// LogicalRenderingDevice	//////////////////////////
	//////////////////////////////////////////////////////////////////////////


	LogicalRenderingDevice::LogicalRenderingDevice(const std::shared_ptr<PhysicalRenderingDevice>& physicalDevice, VkPhysicalDeviceFeatures features)
	{

	}

	LogicalRenderingDevice::~LogicalRenderingDevice()
	{

	}


	void LogicalRenderingDevice::FlushOntoScreen(VkCommandBuffer buffer)
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


		VkSemaphore imageReadySemaphore, renderFinishedSemaphore;
		VkFence framesInFlightFence;
		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &imageReadySemaphore);
		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
		vkCreateFence(m_Device, &fenceInfo, nullptr, &framesInFlightFence);

		VkSemaphore waitSemaphores[] = { imageReadySemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkQueueSubmit(m_RenderingQueue, 1, &submitInfo, framesInFlightFence);
		vkWaitForFences(m_Device, 1, &framesInFlightFence, VK_TRUE, UINT64_MAX);

		vkDestroySemaphore(m_Device, imageReadySemaphore, nullptr);
		vkDestroySemaphore(m_Device, renderFinishedSemaphore, nullptr);
		vkDestroyFence(m_Device, framesInFlightFence, nullptr);

// 		VkPresentInfoKHR presentInfo{};
// 		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
// 
// 		presentInfo.waitSemaphoreCount = 1;
// 		presentInfo.pWaitSemaphores = signalSemaphores;

// 		VkSwapchainKHR swapChains[] = { m_SwapChain };
// 		presentInfo.swapchainCount = 1;
// 		presentInfo.pSwapchains = swapChains;
// 
// 		presentInfo.pImageIndices = &imageIndex;

//		vkQueuePresentKHR(m_RenderingQueue, &presentInfo);

	}

	void LogicalRenderingDevice::Shutdown()
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		vkDeviceWaitIdle(m_Device);
		vkDestroyDevice(m_Device, nullptr);

	}

}