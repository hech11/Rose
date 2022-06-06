#pragma once


#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include <memory>

namespace Rose
{

	struct QueueFamily
	{
		int32_t Graphics, Transfer; // TODO: we don't support compute cmd queues yet....
	};

	class PhysicalRenderingDevice
	{
		public:
			PhysicalRenderingDevice();
			~PhysicalRenderingDevice();


		private:
			VkPhysicalDevice m_PhysicalDevice{};
			VkPhysicalDeviceProperties m_PhysicalDeviceProps{};
			VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures{};
			VkPhysicalDeviceMemoryProperties m_PhysicalMemProps{};

			QueueFamily m_QueueFamilyIndicies;



	};

}