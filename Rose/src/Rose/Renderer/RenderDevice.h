#pragma once


#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Rose
{

	struct QueueFamily
	{
		int32_t Graphics, Transfer; // TODO: we don't support compute cmd queues yet....
		int32_t Present;
	};

	class PhysicalRenderingDevice
	{
		public:
			PhysicalRenderingDevice();
			~PhysicalRenderingDevice();

			const QueueFamily& GetQueueFamily() const { return m_QueueFamilyIndicies; }
			const VkPhysicalDevice& GetDevice() const { return m_PhysicalDevice; } 


		private:
			VkPhysicalDevice m_PhysicalDevice{};
			VkPhysicalDeviceProperties m_PhysicalDeviceProps{};
			VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures{};
			VkPhysicalDeviceMemoryProperties m_PhysicalMemProps{};

			std::vector<VkDeviceQueueCreateInfo> m_DeviceQueueInfos;

			QueueFamily m_QueueFamilyIndicies;
			float queuePriority = 0.0f;

			friend class LogicalRenderingDevice;
	};


	class LogicalRenderingDevice 
	{
		public :
			LogicalRenderingDevice(const std::shared_ptr<PhysicalRenderingDevice>& physicalDevice, VkPhysicalDeviceFeatures features);

			~LogicalRenderingDevice();

			void FlushOntoScreen(VkCommandBuffer buffer);
			void Shutdown();

			VkCommandPool GetCommandPool() { return m_CommandPool; }

			void BeginCommand(VkCommandBuffer& buffer);

			VkDevice& GetDevice() { return m_Device; }
			uint32_t GetImageIndex() {
				return m_ImageIndex;
			}


		private :
			VkDevice m_Device;
			std::shared_ptr<PhysicalRenderingDevice> m_PhysicalDevice;
			VkPhysicalDeviceFeatures m_DeviceFeatures;

			VkCommandPool m_CommandPool;
			VkQueue m_RenderingQueue;

			VkSemaphore m_ImageReadySemaphore, m_RenderFinishedSemaphore;
			VkFence m_FramesInFlightFence;

			uint32_t m_ImageIndex;
	};
}