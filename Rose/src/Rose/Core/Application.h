#pragma once


#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


#include <vector>

namespace Rose
{

	class Application
	{
		public :
			Application();
			~Application();

			void Run();


			const GLFWwindow* GetWindow() const;
			GLFWwindow* GetWindow();
		public :

			static Application& Get() {
				static Application INSTANCE;
				return INSTANCE;
			}


		private :
			void MakeWindow();
			void CreateVulkanInstance();
			bool CheckValidationLayerSupport();

			void ChoosePhysicalDevice();

			void CreateWinGLFWSurface();

			void CleanUp();


			std::vector<const char*> GetRequiredExtensions();

		private :
			GLFWwindow* m_Window = nullptr;
			VkInstance m_VKInstance;
			VkPhysicalDevice m_VKPhysicalDevice = VK_NULL_HANDLE;
			VkDevice m_VKLogicalDebice;
			VkQueue m_RenderingQueue;


			VkDebugUtilsMessengerEXT m_DebugMessagerCallback;
			VkSurfaceKHR m_VKWinSurface;


			const std::vector<const char*> m_ValidationLayerNames = {
				"VK_LAYER_KHRONOS_validation"
			};


	};
}