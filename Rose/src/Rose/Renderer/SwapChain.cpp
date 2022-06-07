#include "SwapChain.h"
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
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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