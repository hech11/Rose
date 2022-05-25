#pragma once

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>]
#include <vector>

struct GLFWwindow;
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
			void CreateWindow();
			void CreateVulkanInstance();
			bool CheckValidationLayerSupport();
			void CleanUp();

			void InitCallbacks();

			std::vector<const char*> GetRequiredExtensions();

		private :
			GLFWwindow* m_Window = nullptr;
			VkInstance m_VKInstance;
			VkDebugUtilsMessengerEXT m_DebugMessagerCallback;

			const std::vector<const char*> m_ValidationLayerNames = {
				"VK_LAYER_KHRONOS_validation"
			};


	};
}