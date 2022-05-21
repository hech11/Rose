#include "Test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>


#include <iostream>
#include <vector>

#include <glfw/glfw3.h>

namespace Rose
{
	void Test()
	{
		int initglfw = glfwInit();
		if (!initglfw)
		{
			std::cout << "failed to init glfw!\n";
		}

		VkInstance instance;

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
		createInfo.enabledLayerCount = 0;

		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		uint32_t extensionCount = 0;

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		std::cout << "available extensions:\n";

		for (const auto& extension : extensions) {
			std::cout << '\t' << extension.extensionName << '\n';
		}


		glm::vec2 xy = { 124.0f, 123412.0f };

		std::cout << xy.x << ", " << xy.y << std::endl;
		std::cout << "Testing Rose Library!\n";


		vkDestroyInstance(instance, nullptr);
		if (initglfw)
			glfwTerminate();


	}
}