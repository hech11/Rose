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


		CreateImageViews();

		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPoolAndBuffer();


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

		//vkDeviceWaitIdle(m_RenderingContext->);
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
		m_RenderingContext = std::make_shared<RendererContext>();
		m_RenderingContext->Init();
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

			vkCreateImageView(m_RenderingContext->GetLogicalDevice()->GetDevice(), &info, nullptr, &m_SwapChainImageViews[i]);
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

			vkCreateFramebuffer(m_RenderingContext->GetLogicalDevice()->GetDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]);

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
		createInfo.queueFamilyIndex = m_RenderingContext->GetPhysicalDevice()->GetQueueFamily().Graphics;

		vkCreateCommandPool(m_RenderingContext->GetLogicalDevice()->GetDevice(), &createInfo, nullptr, &m_VKCommandPool);

		CreateVertexBuffer();

		CreateIndexBuffer();


		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_VKCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(m_RenderingContext->GetLogicalDevice()->GetDevice(), &allocInfo, &m_VKCommandBuffer);


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
		//vkWaitForFences(m_VKLogicalDevice, 1, &m_FramesInFlightFence, VK_TRUE, UINT64_MAX);
		//vkResetFences(m_VKLogicalDevice, 1, &m_FramesInFlightFence);

		uint32_t imageIndex;
		//vkAcquireNextImageKHR(m_VKLogicalDevice, m_SwapChain, UINT64_MAX, m_ImageReadySemaphore, VK_NULL_HANDLE, &imageIndex);
		vkResetCommandBuffer(m_VKCommandBuffer, 0);

		RecordCommandBuffer(imageIndex);

		m_RenderingContext->GetLogicalDevice()->FlushOntoScreen(m_VKCommandBuffer);

	}

	SwapChainDetails Application::QuerySwapChainSupport(VkPhysicalDevice device)
	{
	    SwapChainDetails result;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_RenderingContext->GetPhysicalDevice()->GetDevice(), m_VKWinSurface, &result.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_RenderingContext->GetPhysicalDevice()->GetDevice(), m_VKWinSurface, &formatCount, nullptr);

		result.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_RenderingContext->GetPhysicalDevice()->GetDevice(), m_VKWinSurface, &formatCount, result.Formats.data());

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_RenderingContext->GetPhysicalDevice()->GetDevice(), m_VKWinSurface, &presentModeCount, nullptr);


		result.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_RenderingContext->GetPhysicalDevice()->GetDevice(), m_VKWinSurface, &presentModeCount, result.PresentModes.data());


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

		//vkCreateWin32SurfaceKHR(m_VKInstance, &createInfo, nullptr, &m_VKWinSurface);

//		glfwCreateWindowSurface(m_VKInstance, m_Window, nullptr, &m_VKWinSurface);
	}

	void Application::CleanUp()
	{

		m_VBO->FreeMemory();
		m_IBO->FreeMemory();


		vkDestroyCommandPool(m_RenderingContext->GetLogicalDevice()->GetDevice(), m_VKCommandPool, nullptr);

		m_Shader->DestroyPipeline();

		for (auto fb : m_Framebuffers) {
			vkDestroyFramebuffer(m_RenderingContext->GetLogicalDevice()->GetDevice(), fb, nullptr);
		}

		for (auto view : m_SwapChainImageViews) {
			vkDestroyImageView(m_RenderingContext->GetLogicalDevice()->GetDevice(), view, nullptr);
		}


		vkDestroySwapchainKHR(m_RenderingContext->GetLogicalDevice()->GetDevice(), m_SwapChain, nullptr);
		vkDestroyDevice(m_RenderingContext->GetLogicalDevice()->GetDevice(), nullptr);

		m_RenderingContext->Shutdown();

		glfwDestroyWindow(m_Window);
		glfwTerminate();

	}



	}