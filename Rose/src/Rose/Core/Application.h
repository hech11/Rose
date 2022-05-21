#pragma once

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>

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
			void CleanUp();

		private :
			GLFWwindow* m_Window = nullptr;
			VkInstance m_VKInstance;

	};
}