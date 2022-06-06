#pragma once

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>


namespace Rose
{


	class RendererContext
	{

		public :
			RendererContext();
			~RendererContext();

			void Init();
			void Shutdown();

		public :
			static VkInstance& GetInstance() { return s_VKInstance; }

		private :
			VkPhysicalDevice m_PhysicalDevice;
			VkDevice m_LogicalDevice;

			static VkInstance s_VKInstance;
			VkDebugUtilsMessengerEXT m_DebugMessagerCallback = VK_NULL_HANDLE;


	};


}