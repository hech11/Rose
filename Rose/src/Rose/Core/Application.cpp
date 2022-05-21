#include "Application.h"

#include "Log.h"
#include <glfw/glfw3.h>
#include <vector>



namespace Rose
{

	Application::Application()
	{

		CreateWindow();
		CreateVulkanInstance();
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

		uint32_t extensionCount = 0;


		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		
		createInfo.enabledLayerCount = 0;

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VKInstance);
		if (result != VK_SUCCESS) {
			printf("Failed to create VK instance!\n");
		}
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		LOG("available extensions:\n");

		for (const auto& extension : extensions) {
			printf("\t%s\n", extension.extensionName);
		}

	}

	void Application::CleanUp()
	{

		vkDestroyInstance(m_VKInstance, nullptr);

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

}