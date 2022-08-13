#include "RenderDevice.h"
#include "RendererContext.h"

#include "Rose/Core/Log.h"

#include <vector>
#include "Rose/Core/Application.h"

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


					m_QueueFamilyIndicies.Present = i;
					found = 1;
					i++;
					continue;
				}
			}

			if (queueTypesToQuery & VK_QUEUE_TRANSFER_BIT)
			{
				if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && found == 1)
				{
					m_QueueFamilyIndicies.Transfer = i;
					found = 2;
					i++;
					continue;
				}
			}

			if (found == 2)
				break;
			i++;
		}




		if (queueTypesToQuery & VK_QUEUE_GRAPHICS_BIT)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = m_QueueFamilyIndicies.Graphics;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			m_DeviceQueueInfos.push_back(queueCreateInfo);
		}

		if (queueTypesToQuery & VK_QUEUE_TRANSFER_BIT)
		{
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_QueueFamilyIndicies.Transfer;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &queuePriority;

			m_DeviceQueueInfos.push_back(queueInfo);

		}

	


	}

	PhysicalRenderingDevice::~PhysicalRenderingDevice()
	{

	}








	VkFormat PhysicalRenderingDevice::FindDepthFormat()
	{
		return FindSupportedFormats({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat PhysicalRenderingDevice::FindSupportedFormats(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}


		ASSERT();
		return VK_FORMAT_MAX_ENUM;
	}

	//////////////////////////////////////////////////////////////////////////
	/////////////////////// LogicalRenderingDevice	//////////////////////////
	//////////////////////////////////////////////////////////////////////////


	LogicalRenderingDevice::LogicalRenderingDevice(const std::shared_ptr<PhysicalRenderingDevice>& physicalDevice, VkPhysicalDeviceFeatures features)
	{
		m_PhysicalDevice = physicalDevice;
		std::vector<const char*> deviceExtensions;
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME); // TODO: we just assume we already have access to the swapchain...

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(physicalDevice->m_DeviceQueueInfos.size());;
		deviceCreateInfo.pQueueCreateInfos = physicalDevice->m_DeviceQueueInfos.data();
		deviceCreateInfo.pEnabledFeatures = &features;

		
		if (deviceExtensions.size() > 0)
		{
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		VkResult result = vkCreateDevice(m_PhysicalDevice->GetDevice(), &deviceCreateInfo, nullptr, &m_Device);

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_PhysicalDevice->GetQueueFamily().Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr, &m_CommandPool);

		vkGetDeviceQueue(m_Device, m_PhysicalDevice->GetQueueFamily().Graphics, 0, &m_RenderingQueue);


		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageReadySemaphore);
		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore);
		vkCreateFence(m_Device, &fenceInfo, nullptr, &m_FramesInFlightFence);

	}

	LogicalRenderingDevice::~LogicalRenderingDevice()
	{

	}


	void LogicalRenderingDevice::FlushOntoScreen(VkCommandBuffer buffer)
	{


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageReadySemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkQueueSubmit(m_RenderingQueue, 1, &submitInfo, m_FramesInFlightFence);
		vkWaitForFences(m_Device, 1, &m_FramesInFlightFence, VK_TRUE, UINT64_MAX);

		
 		VkPresentInfoKHR presentInfo{};
 		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
 
 		presentInfo.waitSemaphoreCount = 1;
 		presentInfo.pWaitSemaphores = signalSemaphores;

 		VkSwapchainKHR swapChains[] = { Application::Get().GetSwapChain()->GetSwapChain() };
 		presentInfo.swapchainCount = 1;
 		presentInfo.pSwapchains = swapChains;
 
 		presentInfo.pImageIndices = &m_ImageIndex;

		vkQueuePresentKHR(m_RenderingQueue, &presentInfo);

	}

	void LogicalRenderingDevice::Shutdown()
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		vkDestroySemaphore(m_Device, m_ImageReadySemaphore, nullptr);
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
		vkDestroyFence(m_Device, m_FramesInFlightFence, nullptr);

		vkDeviceWaitIdle(m_Device);
		vkDestroyDevice(m_Device, nullptr);


	}

	void LogicalRenderingDevice::BeginCommand(VkCommandBuffer& buffer)
	{
		vkWaitForFences(m_Device, 1, &m_FramesInFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_FramesInFlightFence);

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(m_Device, Application::Get().GetSwapChain()->GetSwapChain(), UINT64_MAX, m_ImageReadySemaphore, VK_NULL_HANDLE, &imageIndex);
		m_ImageIndex = imageIndex;
		vkResetCommandBuffer(buffer, 0);
	}

}