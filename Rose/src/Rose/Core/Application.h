#pragma once


#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


#include <vector>
#include <array>
#include <optional>
#include <memory>
#include <glm/glm.hpp>

#include "Rose/Renderer/API/Shader.h"
#include "Rose/Renderer/API/VertexBuffer.h"
#include "Rose/Renderer/API/IndexBuffer.h"
#include "Rose/Renderer/RendererContext.h"


namespace Rose
{
	struct VertexData
	{
		glm::vec2 Position;
		glm::vec3 Color;

		static VkVertexInputBindingDescription GetBindingDescription() {

			VkVertexInputBindingDescription result{};
			result.binding = 0;
			result.stride = sizeof(VertexData);
			result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return result;
		}

		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescription()
		{
			std::array<VkVertexInputAttributeDescription, 2> result{};

			result[0].binding = 0;
			result[0].location = 0;
			result[0].format = VK_FORMAT_R32G32_SFLOAT;
			result[0].offset = offsetof(VertexData, Position);


			result[1].binding = 0;
			result[1].location = 1;
			result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			result[1].offset = offsetof(VertexData, Color);

			return result;

		}
	};

	struct SwapChainDetails {
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};



	class Application
	{
		public :
			Application();
			~Application();


			void Run();


			const GLFWwindow* GetWindow() const;
			GLFWwindow* GetWindow();

			const std::shared_ptr<RendererContext>& GetContext() const { return m_RenderingContext; }
			VkExtent2D GetSwapChainExtent() const { return m_SwapChainExtent; }
			VkFormat GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }

		public :

			static Application& Get() {
				return *s_INSTANCE;
			}


		private :
			void MakeWindow();
			void CreateVulkanInstance();

			void CreateImageViews();

			void CreateGraphicsPipeline();
			void CreateFramebuffers();

			void CreateVertexBuffer();
			void CreateIndexBuffer();

			void CreateCommandPoolAndBuffer();

			void RecordCommandBuffer(uint32_t imageIndex);


			void DrawOntoScreen();

			SwapChainDetails QuerySwapChainSupport(VkPhysicalDevice device);

			VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
			VkPresentModeKHR  ChooseSwapPresentMode(const std::vector<VkPresentModeKHR >& presents);
			VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);


			void CreateWinGLFWSurface();

			void CleanUp();


		private :
			GLFWwindow* m_Window = nullptr;

			std::shared_ptr<RendererContext> m_RenderingContext;

			VkSwapchainKHR m_SwapChain;
			std::vector<VkImage> m_SwapChainImages;
			std::vector<VkImageView> m_SwapChainImageViews;
			VkFormat m_SwapChainImageFormat;
			VkExtent2D m_SwapChainExtent;


			std::vector<Rose::VertexData> m_VertexData;
			std::vector<uint32_t> m_IndexData;

			std::shared_ptr<Rose::VertexBuffer> m_VBO;
			std::shared_ptr<Rose::IndexBuffer> m_IBO;


			std::vector<VkFramebuffer> m_Framebuffers;

			VkCommandPool m_VKCommandPool;
			VkCommandBuffer m_VKCommandBuffer;

			std::shared_ptr<Shader> m_Shader;

			VkSurfaceKHR m_VKWinSurface;


			const std::vector<const char*> m_ValidationLayerNames = {
				"VK_LAYER_KHRONOS_validation",
			};

			static Application* s_INSTANCE;

	};
}