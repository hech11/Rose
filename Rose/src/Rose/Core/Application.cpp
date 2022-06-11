#include "Application.h"

#include "Log.h"


#include <vector>
#include <set>
#include <algorithm>
#include <stdexcept>
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtx/transform.hpp"

#include "Rose/Renderer/API/VKMemAllocator.h"

#include <imgui/imgui.h>
#include <glfw/glfw3.h>



namespace Rose
{


	Application* Application::s_INSTANCE = nullptr;

	static bool useCamera = false;
	Application::Application()
	{
		s_INSTANCE = this;
		m_ImguiLayer = new ImguiLayer;


		
			
		m_TestModel = std::make_shared<Model>("assets/models/coneandsphere.obj");

		m_IndexData =
		{
			0, 1, 2, 2, 3, 0
		};
		MakeWindow();
		CreateWinGLFWSurface();
		CreateVulkanInstance();

		VKMemAllocator::Init();

		m_Texture = std::make_shared<Rose::Texture2D>("assets/textures/test.png");

		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPoolAndBuffer();


	}

	Application::~Application()
	{
		CleanUp();
	}

	static glm::vec3 quadPos = {0.0f, 0.0f, 0.0f};

	void Application::Run()
	{
		m_ImguiLayer->Init();


		m_Camera = std::make_shared<Rose::PerspectiveCameraController>(glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f));
		m_Camera->GetCam().SetPosition({ 0.0f, 0.0f, -10.0f });
		UniformBufferData ubo;
		



		while (!glfwWindowShouldClose(m_Window))
		{
			m_Camera->OnUpdate(0.016f);

			ubo.Model = glm::translate(glm::mat4(1.0f), quadPos);
			if (useCamera)
			{
				ubo.ViewProj = m_Camera->GetCam().GetProjView();
			} else {
				ubo.ViewProj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f) * glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, -10.0f });
			}


			glfwPollEvents();
			m_Shader->UpdateUniformBuffer(&ubo, sizeof(ubo), 0);
			DrawOntoScreen();

		}

		vkDeviceWaitIdle(m_RenderingContext->GetLogicalDevice()->GetDevice());
	}

	void Application::OnKeyPressedEvent(int key, int action)
	{
		m_Camera->OnKeyPressedEvent(key, action);
	}

	void Application::OnKeyReleasedEvent(int key)
	{

	}

	void Application::OnMouseMovedEvent(int x, int y)
	{
		m_Camera->OnMouseMovedEvent(x, y);

	}

	void Application::OnMouseButtonClickedEvent(int button, int action)
	{
		m_Camera->OnMouseButtonPressedEvent(button, action);

	}

	void Application::OnMouseButtonReleasedEvent(int button)
	{
		m_Camera->OnMouseButtonReleasedEvent(button);

	}

	void Application::OnMouseScrollWheelUsed(float x, float y)
	{
		m_Camera->OnMouseScrollEvent(x, y);

	}


	const GLFWwindow* Application::GetWindow() const
	{
		return m_Window;
	}

	GLFWwindow* Application::GetWindow()
	{
		return m_Window;

	}

	void Application::MakeWindow()
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
		glfwSetWindowUserPointer(m_Window, this);

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mod)
		{
			if (action == GLFW_PRESS)
				Application::Get().OnKeyPressedEvent(key, action);
			else if (action == GLFW_RELEASE)
				Application::Get().OnKeyReleasedEvent(key);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mod)
		{
			if (action == GLFW_PRESS)
				Application::Get().OnMouseButtonClickedEvent(button, action);
			else if (action == GLFW_RELEASE)
				Application::Get().OnMouseButtonReleasedEvent(button);
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double x, double y)
		{
				Application::Get().OnMouseScrollWheelUsed(x, y);

		});
		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
		{
				Application::Get().OnMouseMovedEvent(x, y);

		});

	}

	void Application::CreateVulkanInstance()
	{
		m_RenderingContext = std::make_shared<RendererContext>();
		m_RenderingContext->Init();


		m_SwapChain->SetVKInstance(m_RenderingContext->GetInstance());
		m_SwapChain->CreateWindowSurface(m_Window);
		m_SwapChain->Init(m_RenderingContext->GetInstance(), m_RenderingContext->GetLogicalDevice());
	}



	void Application::CreateGraphicsPipeline()
	{
		ShaderAttributeLayout layout =
		{
			{"a_Position", 0, ShaderMemberType::Float3},
			{"a_Normal", 1, ShaderMemberType::Float3},
			{"a_TexCoord", 2, ShaderMemberType::Float2}
		};
		m_Shader = std::make_shared<Shader>("assets/shaders/main.shader", layout);

	}

	void Application::CreateFramebuffers()
	{
		m_Framebuffers.resize(m_SwapChain->GetImageViews().size());

		for (size_t i = 0; i < m_SwapChain->GetImageViews().size(); i++) {
			VkImageView attachments[] = {
				m_SwapChain->GetImageViews()[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_Shader->GetRenderPass();
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_SwapChain->GetExtent2D().width;
			framebufferInfo.height = m_SwapChain->GetExtent2D().height;
			framebufferInfo.layers = 1;

			vkCreateFramebuffer(m_RenderingContext->GetLogicalDevice()->GetDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]);

		}
	}

	void Application::CreateVertexBuffer()
	{

		for (auto& mesh : m_TestModel->GetMeshes())
		{
			auto& verticies = mesh.Triangles;
			std::vector<Vertex> verts;
			for (auto triangle : verticies)
			{
				verts.push_back(triangle.Verticies[0]);
				verts.push_back(triangle.Verticies[1]);
				verts.push_back(triangle.Verticies[2]);
			}

			float size = sizeof(Vertex) * verts.size();
			m_VBOs.push_back(std::make_shared<Rose::VertexBuffer>(verts.data(), size));
		}

	}

	void Application::CreateIndexBuffer()
	{

		for (auto& mesh : m_TestModel->GetMeshes())
		{
			auto& indicies = mesh.Indicies;
			m_IBOs.push_back(std::make_shared<Rose::IndexBuffer>(indicies.data(), sizeof(indicies[0]) * indicies.size()));
		}

		
	}

	void Application::CreateCommandPoolAndBuffer()
	{

		CreateVertexBuffer();
		CreateIndexBuffer();


		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_RenderingContext->GetLogicalDevice()->GetCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(m_RenderingContext->GetLogicalDevice()->GetDevice(), &allocInfo, &m_VKCommandBuffer);



	}


	void Application::RecordCommandBuffer(uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;


		vkBeginCommandBuffer(m_VKCommandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_Shader->GetRenderPass();
		renderPassInfo.framebuffer = m_Framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetExtent2D();

		VkClearValue clearColor = { {{0.5f, 0.1f, 0.1f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_VKCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader->GetGrahpicsPipeline());

		for (int i = 0; i < m_VBOs.size(); i++)
		{


			VkBuffer vbos[] = { m_VBOs[i]->GetBufferID() };
			VkDeviceSize offset[] = { 0 };

			vkCmdBindVertexBuffers(m_VKCommandBuffer, 0, 1, vbos, offset);
			vkCmdBindIndexBuffer(m_VKCommandBuffer, m_IBOs[i]->GetBufferID(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader->GetPipelineLayout(), 0, 1, &m_Shader->GetDescriptorSet(), 0, nullptr);
			vkCmdDrawIndexed(m_VKCommandBuffer, m_TestModel->GetMeshes()[i].Indicies.size(), 1, 0, 0, 0);



		}
		OnImguiRender();
		m_ImguiLayer->End();

		vkCmdEndRenderPass(m_VKCommandBuffer);
		vkEndCommandBuffer(m_VKCommandBuffer);


	}

	void Application::DrawOntoScreen()
	{
		m_RenderingContext->GetLogicalDevice()->BeginCommand(m_VKCommandBuffer);
		m_ImguiLayer->Begin();
		RecordCommandBuffer(m_RenderingContext->GetLogicalDevice()->GetImageIndex());


		m_RenderingContext->GetLogicalDevice()->FlushOntoScreen(m_VKCommandBuffer);

	}

	

	void Application::CreateWinGLFWSurface()
	{
 		m_SwapChain = std::make_shared<SwapChain>();

	}

	void Application::CleanUp()
	{

		for (auto& vbo : m_VBOs)
		{
			vbo->FreeMemory();
		}
		for (auto& ibo : m_IBOs)
		{
			ibo->FreeMemory();
		}



		m_Shader->DestroyPipeline();

		for (auto fb : m_Framebuffers) {
			vkDestroyFramebuffer(m_RenderingContext->GetLogicalDevice()->GetDevice(), fb, nullptr);
		}

		m_SwapChain->Destroy();


		m_ImguiLayer->Shutdown();
		m_Texture->Destroy();

		VKMemAllocator::Shutdown();

		m_RenderingContext->GetLogicalDevice()->Shutdown();
		m_RenderingContext->Shutdown();




		glfwDestroyWindow(m_Window);
		glfwTerminate();

	}



	void Application::OnImguiRender()
	{
		ImGui::Begin("Test window!");
		ImGui::Text("This is some text");
		ImGui::SliderFloat3("quad pos", &quadPos.x, -10.0f, 10.0f);
		ImGui::Checkbox("Use camera", &useCamera);
		ImGui::End();
	}

}