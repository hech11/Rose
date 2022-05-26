#include "Application.h"

#include "Log.h"
#include <glfw/glfw3.h>
#include <vector>



namespace Rose
{


	namespace callbacks
	{


		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMSGRCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
		{

			if(messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
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

	Application::Application()
	{

		CreateWindow();
		CreateVulkanInstance();
		ChoosePhysicalDevice();
		
	}

	Application::~Application()
	{
		CleanUp();
	}

	void Application::Run()
	{

		while (!glfwWindowShouldClose(m_Window))
		{

			glfwPollEvents();
			glfwSwapBuffers(m_Window);

		}
	}

	const GLFWwindow* Application::GetWindow() const
	{
		return m_Window;
	}

	GLFWwindow* Application::GetWindow()
	{
		return m_Window;

	}

	void Application::CreateWindow()
	{
		if (!glfwInit())
		{
			LOG("Failed to init GLFW!\n");
		}

		m_Window = glfwCreateWindow(1600, 900, "Vulkan App", nullptr, nullptr);
		if (!m_Window)
		{
			LOG("Failed to create glfw window!\n");
		}
		glfwSwapInterval(1);

	}

	void Application::CreateVulkanInstance()
	{

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "test";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;


		auto ext = GetRequiredExtensions();

		
		createInfo.enabledExtensionCount = ext.size();
		createInfo.ppEnabledExtensionNames = ext.data();
		


		createInfo.enabledLayerCount = (uint32_t)m_ValidationLayerNames.size();
		createInfo.ppEnabledLayerNames = m_ValidationLayerNames.data();


		VkDebugUtilsMessengerCreateInfoEXT debugInfo{};

		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = callbacks::DebugMSGRCallback;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VKInstance);


		callbacks::CreateDebugUtilsMessengerEXT(m_VKInstance, &debugInfo, nullptr, &m_DebugMessagerCallback);

		if (result != VK_SUCCESS) {
			printf("Failed to create VK instance!\n");
		}


		uint32_t extensionCount = 0;
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		LOG("available extensions:\n");

		for (const auto& extension : extensions) {
			printf("\t%s\n", extension.extensionName);
		}


		
	}

	bool Application::CheckValidationLayerSupport()
	{
		uint32_t count;
		vkEnumerateInstanceLayerProperties(&count, nullptr);

		std::vector<VkLayerProperties> actualLayers;
		vkEnumerateInstanceLayerProperties(&count, actualLayers.data());


		for(const auto& name : m_ValidationLayerNames)
		{
			bool found = false;
			for (const auto& props : actualLayers)
			{
				if (!strcmp(name, props.description))
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;

		}

		return true;

	}

	void Application::ChoosePhysicalDevice()
	{
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(m_VKInstance, &physicalDeviceCount, nullptr);

		std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(m_VKInstance, &physicalDeviceCount, devices.data());


		m_VKPhysicalDevice = devices[0]; // not great
		//validate if the device is useable?


		// check features once added new API calls to vulkan
		VkPhysicalDeviceFeatures deviceFeatures{};

		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(devices[0], &deviceProps);

		// print info about the device props here

		//  find queue family 

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_VKPhysicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_VKPhysicalDevice, &queueFamilyCount, queueFamilies.data());

		uint32_t graphicsFamily;

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsFamily = i;
			}

			i++;
		}

		// logical devices

		float queuePriority = 1.0f;

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.queueCreateInfoCount = 1;

		vkCreateDevice(m_VKPhysicalDevice, &createInfo, nullptr, &m_VKLogicalDebice);
		vkGetDeviceQueue(m_VKLogicalDebice, graphicsFamily, 0, &m_RenderingQueue);

	}

	void Application::CleanUp()
	{

		callbacks::DestroyDebugUtilsMessengerEXT(m_VKInstance, m_DebugMessagerCallback, nullptr);

		vkDestroyDevice(m_VKLogicalDebice, nullptr);
		vkDestroyInstance(m_VKInstance, nullptr);

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}


	std::vector<const char*> Application::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> result(glfwExtensions, glfwExtensions + glfwExtensionCount);

		result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return result;

	}

}