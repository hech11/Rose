#include "RendererContext.h"
#include <vector>

#include <glfw/glfw3.h>
#include "Rose/Core/LOG.h"



namespace Rose
{

	namespace callbacks
	{
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMSGRCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
		{

			if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
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

	VkInstance RendererContext::s_VKInstance = VK_NULL_HANDLE;


	RendererContext::RendererContext()
	{

	}

	RendererContext::~RendererContext()
	{
	}


	void RendererContext::Init()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Rose";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.pEngineName = "Rose-Renderer";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extentsions(glfwExtensions, glfwExtensions + glfwExtensionCount);


		extentsions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.pNext = nullptr;


		createInfo.enabledExtensionCount = extentsions.size();
		createInfo.ppEnabledExtensionNames = extentsions.data();
		



		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		uint32_t instanceLayerCount;

		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);

		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());

		bool validationLayerPresent = false;
		LOG("VK Instance Layers:");


		for (const VkLayerProperties& layer : instanceLayerProperties)
		{
			LOG("%s\n", layer.layerName);
			if (strcmp(layer.layerName, validationLayerName) == 0)
			{
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent)
		{
			createInfo.ppEnabledLayerNames = &validationLayerName;
			createInfo.enabledLayerCount = 1;
		}
		else
		{
			LOG("VK_LAYER_VALIDATION could not be found!\n");
		}

		

		VkDebugUtilsMessengerCreateInfoEXT debugInfo{};

		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = callbacks::DebugMSGRCallback;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
		VkResult result = vkCreateInstance(&createInfo, nullptr, &s_VKInstance);
		if (result == VK_FALSE)
			LOG("ERROR!\n");


		callbacks::CreateDebugUtilsMessengerEXT(s_VKInstance, &debugInfo, nullptr, &m_DebugMessagerCallback);


		m_PhysicalDevice = std::make_shared<PhysicalRenderingDevice>();
		VkPhysicalDeviceFeatures enabledFeatures;
		memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
		enabledFeatures.samplerAnisotropy = true;
		enabledFeatures.wideLines = true;
		enabledFeatures.fillModeNonSolid = true;
		enabledFeatures.independentBlend = true;
		enabledFeatures.pipelineStatisticsQuery = true;
		m_LogicalDevice = std::make_shared<LogicalRenderingDevice>(m_PhysicalDevice, enabledFeatures);


		
	}

	void RendererContext::Shutdown()
	{
		callbacks::DestroyDebugUtilsMessengerEXT(s_VKInstance, m_DebugMessagerCallback, nullptr);

		vkDestroyInstance(s_VKInstance, nullptr);
	}

}