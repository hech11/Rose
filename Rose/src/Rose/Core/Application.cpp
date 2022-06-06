#include "Application.h"

#include "Log.h"


#include <vector>
#include <set>
#include <algorithm>
#include <stdexcept>
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtx/transform.hpp"


namespace Rose
{


	namespace callbacks
	{


		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMSGRCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
		{

			if(messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				LOG("validation layer: %s: %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);	
			return VK_FALSE;
		}

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else {
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debugMessenger, pAllocator);
			}
		}
	}

	Application* Application::s_INSTANCE = nullptr;



	
	Application::Application()
	{
		s_INSTANCE = this;

		
			
			
		m_VertexData = {
			{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
			{{ -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
		};

		m_IndexData =
		{
			0, 1, 2, 2, 3, 0
		};
		MakeWindow();
		CreateVulkanInstance();
		CreateWinGLFWSurface();


		ChoosePhysicalDevice();
		CreateImageViews();

		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPoolAndBuffer();
		CreateSyncObjs();


	}

	Application::~Application()
	{
		CleanUp();
	}

	void Application::Run()
	{

		UniformBufferData ubo;
		ubo.ViewProj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f) * glm::translate(glm::mat4(1.0f), { 0.5f, 0.0f, -10.0f });
		glm::vec3 pos = {0.0f, 0.0f, 0.0f};
		static float increment = 0.0f;

		while (!glfwWindowShouldClose(m_Window))
		{

			glfwPollEvents();

			ubo.Model = glm::translate(glm::mat4(1.0f), pos);

			pos.x = sin(increment);
			pos.y = cos(increment);
			pos.z = sin(increment);
			increment+=0.0016f*15.0f;

			m_Shader->UpdateUniformBuffer(ubo);
			DrawOntoScreen();

		}

		vkDeviceWaitIdle(m_VKLogicalDevice);
	}

	const GLFWwindow* Application::GetWindow() const
	{
		return m_Window;
	}

	GLFWwindow* Application::GetWindow()
	{
		return m_Window;

	}

	void Application::MakeWindow()
	{
		if (!glfwInit())
		{
			LOG("Failed to init GLFW!\n");
		}

		m_Window = glfwCreateWindow(1600, 900, "Vulkan App", nullptr, nullptr);
		if (!m_Window)
		{
			LOG("Failed to create glfw window!\n");
		}
		glfwSwapInterval(1);

	}

	void Application::CreateVulkanInstance()
	{

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "test";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;


		auto ext = GetRequiredExtensions();

		
		createInfo.enabledExtensionCount = ext.size();
		createInfo.ppEnabledExtensionNames = ext.data();
		


		createInfo.enabledLayerCount = (uint32_t)m_ValidationLayerNames.size();
		createInfo.ppEnabledLayerNames = m_ValidationLayerNames.data();


		VkDebugUtilsMessengerCreateInfoEXT debugInfo{};

		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = callbacks::DebugMSGRCallback;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VKInstance);


		callbacks::CreateDebugUtilsMessengerEXT(m_VKInstance, &debugInfo, nullptr, &m_DebugMessagerCallback);

		if (result != VK_SUCCESS) {
			printf("Failed to create VK instance!\n");
		}


		uint32_t extensionCount = 0;
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		LOG("available extensions:\n");

		for (const auto& extension : extensions) {
			printf("\t%s\n", extension.extensionName);
		}


		
	}

	bool Application::CheckValidationLayerSupport()
	{
		uint32_t count;
		vkEnumerateInstanceLayerProperties(&count, nullptr);

		std::vector<VkLayerProperties> actualLayers;
		vkEnumerateInstanceLayerProperties(&count, actualLayers.data());


		for(const auto& name : m_ValidationLayerNames)
		{
			bool found = false;
			for (const auto& props : actualLayers)
			{
				if (!strcmp(name, props.description))
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;

		}

		return true;

	}

	void Application::ChoosePhysicalDevice()
	{
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(m_VKInstance, &physicalDeviceCount, nullptr);

		std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(m_VKInstance, &physicalDeviceCount, devices.data());


		m_VKPhysicalDevice = devices[0]; // not great
		//validate if the device is useable?


		// check features once added new API calls to vulkan
		VkPhysicalDeviceFeatures deviceFeatures{};

		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(devices[0], &deviceProps);

		// print info about the device props here

		//  find queue family 

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_VKPhysicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_VKPhysicalDevice, &queueFamilyCount, queueFamilies.data());

		

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsFamily = i;
			}

			uint32_t presentSupport;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_VKPhysicalDevice, i, m_VKWinSurface, &presentSupport);
			if (presentSupport)
			{
				presentFamily = i;
			}

			if (graphicsFamily.has_value() && presentFamily.has_value())
				break;


			i++;
		}

		// logical devices

		float queuePriority = 1.0f;

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.queueCreateInfoCount = 1;


		// getting device extensions.... we assume we already support the swap chain..
		std::vector<const char*> deviceExtentions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(m_VKPhysicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> deviceExtsProps(extensionCount);
		vkEnumerateDeviceExtensionProperties(m_VKPhysicalDevice, nullptr, &extensionCount, deviceExtsProps.data());


		std::set<std::string> requiredExts(deviceExtentions.begin(), deviceExtentions.end());

		for (const auto& extension : deviceExtsProps) {
			requiredExts.erase(extension.extensionName);
		}
		for (const auto& names : requiredExts)
		{
			LOG("we support these extentions: %s\n", names.c_str());
		}

		createInfo.enabledExtensionCount = deviceExtentions.size();
		createInfo.ppEnabledExtensionNames = deviceExtentions.data();

		vkCreateDevice(m_VKPhysicalDevice, &createInfo, nullptr, &m_VKLogicalDevice);
		vkGetDeviceQueue(m_VKLogicalDevice, graphicsFamily.value(), 0, &m_RenderingQueue);



		// creating swapchain

		SwapChainDetails swapChainDetails = QuerySwapChainSupport(m_VKPhysicalDevice);


		uint32_t imageCount = swapChainDetails.Capabilities.minImageCount + 1;

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainDetails.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainDetails.PresentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainDetails.Capabilities);

		VkSwapchainCreateInfoKHR swapChainInfo{};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = m_VKWinSurface;

		swapChainInfo.minImageCount = imageCount;
		swapChainInfo.imageFormat = surfaceFormat.format;
		swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainInfo.imageExtent = extent;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// concurrent only
		uint32_t familyIndicies[] = { graphicsFamily.value(), presentFamily.value() };
		if(graphicsFamily.value() != presentFamily.value())
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

		vkCreateSwapchainKHR(m_VKLogicalDevice, &swapChainInfo, nullptr, &m_SwapChain);

		uint32_t swapchainImageCount;
		vkGetSwapchainImagesKHR(m_VKLogicalDevice, m_SwapChain, &swapchainImageCount, nullptr);
		m_SwapChainImages.resize(swapchainImageCount);
		vkGetSwapchainImagesKHR(m_VKLogicalDevice, m_SwapChain, &swapchainImageCount, m_SwapChainImages.data());

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;


	}

	void Application::CreateImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());

		for (uint32_t i = 0; i < m_SwapChainImages.size(); i++)
		{

			VkImageViewCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.image = m_SwapChainImages[i];
			info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			info.format = m_SwapChainImageFormat;
			info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			info.subresourceRange.baseMipLevel = 0;
			info.subresourceRange.levelCount = 1;
			info.subresourceRange.baseArrayLayer = 0;
			info.subresourceRange.layerCount = 1;

			vkCreateImageView(m_VKLogicalDevice, &info, nullptr, &m_SwapChainImageViews[i]);
		}
	}

	void Application::CreateGraphicsPipeline()
	{
		m_Shader = std::make_shared<Shader>("assets/shaders/test.shader");

	}

	void Application::CreateFramebuffers()
	{
		m_Framebuffers.resize(m_SwapChainImageViews.size());

		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				m_SwapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_Shader->GetRenderPass();
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_SwapChainExtent.width;
			framebufferInfo.height = m_SwapChainExtent.height;
			framebufferInfo.layers = 1;

			vkCreateFramebuffer(m_VKLogicalDevice, &framebufferInfo, nullptr, &m_Framebuffers[i]);

		}
	}

	void Application::CreateVertexBuffer()
	{

		m_VBO = std::make_shared<Rose::VertexBuffer>(m_VertexData.data(), sizeof(m_VertexData[0]) * m_VertexData.size());

	}

	void Application::CreateIndexBuffer()
	{

		m_IBO = std::make_shared<Rose::IndexBuffer>(m_IndexData.data(), sizeof(m_IndexData[0]) * m_IndexData.size());
	}

	void Application::CreateCommandPoolAndBuffer()
	{
		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = graphicsFamily.value();

		vkCreateCommandPool(m_VKLogicalDevice, &createInfo, nullptr, &m_VKCommandPool);

		CreateVertexBuffer();

		CreateIndexBuffer();


		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_VKCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(m_VKLogicalDevice, &allocInfo, &m_VKCommandBuffer);


	}

	void Application::CreateSyncObjs()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		vkCreateSemaphore(m_VKLogicalDevice, &semaphoreInfo, nullptr, &m_ImageReadySemaphore);
		vkCreateSemaphore(m_VKLogicalDevice, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore);
		vkCreateFence(m_VKLogicalDevice, &fenceInfo, nullptr, &m_FramesInFlightFence);


	}

	void Application::RecordCommandBuffer(uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(m_VKCommandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_Shader->GetRenderPass();
		renderPassInfo.framebuffer = m_Framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChainExtent;

		VkClearValue clearColor = { {{0.1f, 0.1f, 0.1f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_VKCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader->GetGrahpicsPipeline());


		VkBuffer vbos[] = { m_VBO->GetBufferID() };
		VkDeviceSize offset[] = { 0 };

		vkCmdBindVertexBuffers(m_VKCommandBuffer, 0, 1, vbos, offset);
		vkCmdBindIndexBuffer(m_VKCommandBuffer, m_IBO->GetBufferID(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader->GetPipelineLayout(), 0, 1, &m_Shader->GetDescriptorSet(), 0, nullptr);
		vkCmdDrawIndexed(m_VKCommandBuffer, m_IndexData.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_VKCommandBuffer);
		vkEndCommandBuffer(m_VKCommandBuffer);

	}

	void Application::DrawOntoScreen()
	{
		vkWaitForFences(m_VKLogicalDevice, 1, &m_FramesInFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_VKLogicalDevice, 1, &m_FramesInFlightFence);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(m_VKLogicalDevice, m_SwapChain, UINT64_MAX, m_ImageReadySemaphore, VK_NULL_HANDLE, &imageIndex);
		vkResetCommandBuffer(m_VKCommandBuffer, 0);


		

		RecordCommandBuffer(imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageReadySemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_VKCommandBuffer;

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkQueueSubmit(m_RenderingQueue, 1, &submitInfo, m_FramesInFlightFence);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(m_RenderingQueue, &presentInfo);

	}

	SwapChainDetails Application::QuerySwapChainSupport(VkPhysicalDevice device)
	{
	    SwapChainDetails result;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VKPhysicalDevice, m_VKWinSurface, &result.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_VKPhysicalDevice, m_VKWinSurface, &formatCount, nullptr);

		result.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_VKPhysicalDevice, m_VKWinSurface, &formatCount, result.Formats.data());

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_VKPhysicalDevice, m_VKWinSurface, &presentModeCount, nullptr);


		result.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_VKPhysicalDevice, m_VKWinSurface, &presentModeCount, result.PresentModes.data());


	    return result;
	}

	VkSurfaceFormatKHR Application::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		for (const auto& availableFormat : formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return formats[0];
	}

	VkPresentModeKHR Application::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR >& presents)
	{
		for (const auto& availablePresentMode : presents) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Application::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(m_Window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void Application::CreateWinGLFWSurface()
	{
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = glfwGetWin32Window(m_Window);
		createInfo.hinstance = GetModuleHandle(nullptr);

		vkCreateWin32SurfaceKHR(m_VKInstance, &createInfo, nullptr, &m_VKWinSurface);

//		glfwCreateWindowSurface(m_VKInstance, m_Window, nullptr, &m_VKWinSurface);
	}

	void Application::CleanUp()
	{

		callbacks::DestroyDebugUtilsMessengerEXT(m_VKInstance, m_DebugMessagerCallback, nullptr);

		m_VBO->FreeMemory();
		m_IBO->FreeMemory();

		vkDestroySemaphore(m_VKLogicalDevice, m_ImageReadySemaphore, nullptr);
		vkDestroySemaphore(m_VKLogicalDevice, m_RenderFinishedSemaphore, nullptr);
		vkDestroyFence(m_VKLogicalDevice, m_FramesInFlightFence, nullptr);

		vkDestroyCommandPool(m_VKLogicalDevice, m_VKCommandPool, nullptr);

		m_Shader->DestroyPipeline();

		for (auto fb : m_Framebuffers) {
			vkDestroyFramebuffer(m_VKLogicalDevice, fb, nullptr);
		}

		for (auto view : m_SwapChainImageViews) {
			vkDestroyImageView(m_VKLogicalDevice, view, nullptr);
		}


		vkDestroySwapchainKHR(m_VKLogicalDevice, m_SwapChain, nullptr);
		vkDestroyDevice(m_VKLogicalDevice, nullptr);

		vkDestroySurfaceKHR(m_VKInstance, m_VKWinSurface, nullptr);
		vkDestroyInstance(m_VKInstance, nullptr);

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}


	std::vector<const char*> Application::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> result(glfwExtensions, glfwExtensions + glfwExtensionCount);

		result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return result;

	}

	}