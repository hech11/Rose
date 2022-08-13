#pragma once




#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>



#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include <vector>
#include <memory>


namespace Rose
{


	struct SwapChainDetails {
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};



	class LogicalRenderingDevice;
	class SwapChain
	{
		public :
			void Init(VkInstance& instance, const std::shared_ptr<LogicalRenderingDevice>& device);
			void CreateWindowSurface(GLFWwindow* window);

			void Destroy();
			void SetVKInstance(VkInstance& instance) { m_VKInstance = instance; }

			VkSwapchainKHR GetSwapChain() { return m_SwapChain; }

			std::vector<VkImageView>& GetImageViews() { return m_ImageViews; }
			VkExtent2D& GetExtent2D() { return m_Extent2D; }

			VkImageView& GetDepthImageView() { return m_DepthImageView; }
			const VkImageView& GetDepthImageView() const { return m_DepthImageView; }

			VkImageView& GetMultisampledColorImageView() { return m_ColorSampledImageView; }
			const VkImageView& GetMultisampledColorImageView() const { return m_ColorSampledImageView; }

			VkFormat GetColorFormat() { return m_ColorFormat; }
			VkSurfaceKHR GetWindowSurface() const { return m_WinSurface; }

		private :
			SwapChainDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice);


			VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
			VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR >& presents);
			VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

			void CreateImageViews();


		private :
			VkSwapchainKHR m_SwapChain;
			std::vector<VkImage> m_SwapChainImages;
			std::vector<VkImageView> m_ImageViews;

			VkImage m_ColorSampledImage;
			VkImageView m_ColorSampledImageView;
			VkDeviceMemory m_ColorSampledDeviceMemory;



			VkImage m_DepthImage;
			VkImageView m_DepthImageView;
			VkDeviceMemory m_DepthDeviceMemory;

			VkExtent2D m_Extent2D;

			VkSurfaceKHR m_WinSurface;


			VkFormat m_ColorFormat;
			VkColorSpaceKHR m_ColorSpace;

			VkInstance m_VKInstance;
			std::shared_ptr<LogicalRenderingDevice> m_LogicalDevice;
				
				
			

	};
}