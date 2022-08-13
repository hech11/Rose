#include "SwapChain.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <glfw/glfw3.h>

#include "RenderDevice.h"
#include "Rose/Core/Application.h"




namespace Rose
{


	void SwapChain::Init(VkInstance& instance, const std::shared_ptr<LogicalRenderingDevice>& device)
	{
		m_VKInstance = instance;
		m_LogicalDevice = device;


		auto& physicalDevice = Application::Get().GetContext()->GetPhysicalDevice();
		auto& winSurface = m_WinSurface;

		SwapChainDetails swapChainDetails = QuerySwapChainSupport(physicalDevice->GetDevice());


		uint32_t imageCount = swapChainDetails.Capabilities.minImageCount + 1;

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainDetails.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainDetails.PresentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainDetails.Capabilities);

		VkSwapchainCreateInfoKHR swapChainInfo{};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = winSurface;

		swapChainInfo.minImageCount = imageCount;
		swapChainInfo.imageFormat = surfaceFormat.format;
		swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainInfo.imageExtent = extent;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// concurrent only

		int32_t graphicsFamily = physicalDevice->GetQueueFamily().Graphics;
		int32_t presentFamily = physicalDevice->GetQueueFamily().Present;

		uint32_t familyIndicies[] = { physicalDevice->GetQueueFamily().Graphics, physicalDevice->GetQueueFamily().Present };
		if (graphicsFamily != presentFamily)
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.queueFamilyIndexCount = 2;
			swapChainInfo.pQueueFamilyIndices = familyIndicies;
		}
		else
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swapChainInfo.preTransform = swapChainDetails.Capabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainInfo.presentMode = presentMode;
		swapChainInfo.clipped = VK_TRUE;


		swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

		vkCreateSwapchainKHR(device->GetDevice(), &swapChainInfo, nullptr, &m_SwapChain);

		uint32_t swapchainImageCount;
		vkGetSwapchainImagesKHR(device->GetDevice(), m_SwapChain, &swapchainImageCount, nullptr);
		m_SwapChainImages.resize(swapchainImageCount);
		vkGetSwapchainImagesKHR(device->GetDevice(), m_SwapChain, &swapchainImageCount, m_SwapChainImages.data());

		m_ColorFormat = surfaceFormat.format;
		m_Extent2D = extent;



		CreateImageViews();


		//TODO: Replace this all with VMA and its own create image pipeline.

		VkFormat depthFormat = physicalDevice->FindDepthFormat();

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_Extent2D.width;
		imageInfo.extent.height = m_Extent2D.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateImage(device->GetDevice(), &imageInfo, nullptr, &m_DepthImage);

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device->GetDevice(), m_DepthImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;


		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice->GetDevice(), &memProperties);
		int memType = 0;
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
				memType = i;
				break;
			}
		}

		allocInfo.memoryTypeIndex = memType;
		vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &m_DepthDeviceMemory);

		vkBindImageMemory(device->GetDevice(), m_DepthImage, m_DepthDeviceMemory, 0);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_DepthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &m_DepthImageView);

	}

	void SwapChain::CreateWindowSurface(GLFWwindow* window)
	{
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = glfwGetWin32Window(Application::Get().GetWindow());
		createInfo.hinstance = GetModuleHandle(nullptr);

		vkCreateWin32SurfaceKHR(m_VKInstance, &createInfo, nullptr, &m_WinSurface);

		auto& physicalDevice = Application::Get().GetContext()->GetPhysicalDevice();
		uint32_t presentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->GetDevice(), physicalDevice->GetQueueFamily().Graphics, m_WinSurface, &presentSupport);
	}

	void SwapChain::Destroy()
	{
		vkDestroyImageView(m_LogicalDevice->GetDevice(), m_DepthImageView, nullptr);
		vkDestroyImage(m_LogicalDevice->GetDevice(), m_DepthImage, nullptr);

		for (auto view : m_ImageViews) {
			vkDestroyImageView(m_LogicalDevice->GetDevice(), view, nullptr);
		}

		vkDestroySwapchainKHR(m_LogicalDevice->GetDevice(), m_SwapChain, nullptr);
		vkDestroySurfaceKHR(m_VKInstance, m_WinSurface, nullptr);
	}

	Rose::SwapChainDetails SwapChain::QuerySwapChainSupport(VkPhysicalDevice physicalDevice)
	{
		VkSurfaceKHR& winSurface = m_WinSurface;
		SwapChainDetails result;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, winSurface, &result.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, winSurface, &formatCount, nullptr);

		result.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, winSurface, &formatCount, result.Formats.data());

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, winSurface, &presentModeCount, nullptr);


		result.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, winSurface, &presentModeCount, result.PresentModes.data());


		return result;
	}


	void SwapChain::CreateImageViews()
	{
		m_ImageViews.resize(m_SwapChainImages.size());

		for (uint32_t i = 0; i < m_SwapChainImages.size(); i++)
		{

			VkImageViewCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.image = m_SwapChainImages[i];
			info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			info.format = m_ColorFormat;
			info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			info.subresourceRange.baseMipLevel = 0;
			info.subresourceRange.levelCount = 1;
			info.subresourceRange.baseArrayLayer = 0;
			info.subresourceRange.layerCount = 1;

			vkCreateImageView(m_LogicalDevice->GetDevice(), &info, nullptr, &m_ImageViews[i]);
		}
	}

	VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		for (const auto& availableFormat : formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return formats[0];
	}

	VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR >& presents)
	{
		for (const auto& availablePresentMode : presents) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width = 1600, height = 900;
			glfwGetFramebufferSize(Application::Get().GetWindow(), &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}


}