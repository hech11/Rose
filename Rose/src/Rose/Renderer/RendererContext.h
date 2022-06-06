#pragma once

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include "RenderDevice.h"


namespace Rose
{


	class RendererContext
	{

		public :
			RendererContext();
			~RendererContext();

			void Init();
			void Shutdown();

			const std::shared_ptr<LogicalRenderingDevice>& GetLogicalDevice() const { return m_LogicalDevice; }
			const std::shared_ptr<PhysicalRenderingDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }

		public :
			static VkInstance& GetInstance() { return s_VKInstance; }

		private :
			std::shared_ptr<PhysicalRenderingDevice> m_PhysicalDevice;
			std::shared_ptr <LogicalRenderingDevice> m_LogicalDevice;

			static VkInstance s_VKInstance;
			VkDebugUtilsMessengerEXT m_DebugMessagerCallback = VK_NULL_HANDLE;


	};


}