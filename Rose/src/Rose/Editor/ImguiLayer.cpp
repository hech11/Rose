#include "ImguiLayer.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include "Rose/Core/Application.h"
#include "../Core/LOG.h"

namespace Rose
{

	static void check_vk_result(VkResult err)
	{
		if (err == 0)
			return;
		fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
		if (err < 0)
			abort();
	}

	void ImguiLayer::Init()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		const auto& window = Application::Get().GetWindow();
		const auto& context = Application::Get().GetContext();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = context->GetInstance();
		init_info.PhysicalDevice = context->GetPhysicalDevice()->GetDevice();
		init_info.Device = context->GetLogicalDevice()->GetDevice();
		init_info.QueueFamily = context->GetPhysicalDevice()->GetQueueFamily().Graphics;
		init_info.Queue = context->GetLogicalDevice()->GetQueue();
		//init_info.PipelineCache = g_PipelineCache;
		init_info.DescriptorPool = Application::Get().GetShader()->GetDescriptorPool();
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = 2;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, Application::Get().GetShader()->GetRenderPass());


		// Upload Fonts
		{
			// Use any command queue
			VkCommandPool command_pool = Application::Get().GetContext()->GetLogicalDevice()->GetCommandPool();
			VkCommandBuffer command_buffer = Application::Get().GetCommandBuffer();

			vkResetCommandPool(Application::Get().GetContext()->GetLogicalDevice()->GetDevice(), command_pool, 0);
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(command_buffer, &begin_info);

			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &command_buffer;
			vkEndCommandBuffer(command_buffer);
			vkQueueSubmit(Application::Get().GetContext()->GetLogicalDevice()->GetQueue(), 1, &end_info, VK_NULL_HANDLE);

			vkDeviceWaitIdle(context->GetLogicalDevice()->GetDevice());
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}

	}

	void ImguiLayer::Shutdown()
	{
		vkDeviceWaitIdle(Application::Get().GetContext()->GetLogicalDevice()->GetDevice());
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImguiLayer::Begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

	}

	void ImguiLayer::End()
	{
		// Rendering
		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
		if (!main_is_minimized)
		{
			ImGui_ImplVulkan_RenderDrawData(main_draw_data, Application::Get().GetCommandBuffer());
		}


		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

}