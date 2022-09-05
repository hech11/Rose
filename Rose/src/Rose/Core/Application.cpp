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


		
			
		
		MakeWindow();
		CreateWinGLFWSurface();
		CreateVulkanInstance();

		VKMemAllocator::Init();
		m_TestModel = std::make_shared<Model>("assets/models/sponza/sponza.gltf");
		//m_TestModel = std::make_shared<Model>("assets/models/sphere.fbx");
		m_SphereModel = std::make_shared<Model>("assets/models/New/Sphere.fbx");
		//m_TestModel = std::make_shared<Model>("assets/models/coneandsphere.obj");



		float skyboxVerts[] =
		{
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  -1.0f,
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  -1.0f,
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  -1.0f,
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  -1.0f,
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  -1.0f,
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  -1.0f,
			// front face		  
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			// left face		  
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
			// right face					 
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
			 // bottom face					    
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,
			 // top face					    
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f, 0.0f,
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f, 0.0f,
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f, 0.0f,
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f, 0.0f,
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f, 0.0f,
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f, 0.0f
		};

		float skyboxV[] =
		{
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
		};

		uint32_t skyboxI[] =
		{
			1,2,6,
			6,5,1,

			0,4,7,
			7,3,0,

			4,5,6,
			6,7,4,

			0,3,2,
			2,1,0,
			
			0,1,5,
			5,4,0,

			3,7,6,
			6,2,3
		};

		ShaderAttributeLayout layout =
		{
			{"a_Position", 0, ShaderMemberType::Float3}
		};


		m_SkyboxShader = std::make_shared<Shader>("assets/shaders/skybox.shader", layout, true);

		MaterialUniform diffuseUniform;




// 		TextureCubeFiles files =
// 		{
// 			{std::string("assets/textures/skybox/sky1/SDR/output_skybox_posx.tga")},
// 			{std::string("assets/textures/skybox/sky1/SDR/output_skybox_negx.tga")},
// 			{std::string("assets/textures/skybox/sky1/SDR/output_skybox_posy.tga")},
// 			{std::string("assets/textures/skybox/sky1/SDR/output_skybox_negy.tga")},
// 			{std::string("assets/textures/skybox/sky1/SDR/output_skybox_posz.tga")},
// 			{std::string("assets/textures/skybox/sky1/SDR/output_skybox_negz.tga")}
// 		};

		TextureCubeFiles files =
		{
			{std::string("assets/textures/skybox/sky1/output_skybox_posx.hdr")},
			{std::string("assets/textures/skybox/sky1/output_skybox_negx.hdr")},
			{std::string("assets/textures/skybox/sky1/output_skybox_posy.hdr")},
			{std::string("assets/textures/skybox/sky1/output_skybox_negy.hdr")},
			{std::string("assets/textures/skybox/sky1/output_skybox_posz.hdr")},
			{std::string("assets/textures/skybox/sky1/output_skybox_negz.hdr")}
		};
		diffuseUniform.Texture3DCube = std::make_shared<TextureCube>(files);
		diffuseUniform.TextureType = PBRTextureType::Rad;
		m_SkyboxUniforms.push_back(diffuseUniform);

		m_SkyboxShader->CreatePipelineAndDescriptorPool(m_SkyboxUniforms);
		
		m_SkyboxVbo = std::make_shared<VertexBuffer>(skyboxV, sizeof(skyboxV));
		m_SkyboxIbo = std::make_shared<IndexBuffer>(skyboxI, sizeof(skyboxI));



		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPoolAndBuffer();


	}

	Application::~Application()
	{
		CleanUp();
	}

	static glm::vec3 spherePos = { 0.0f, 0.0f, 0.0f };
	static glm::vec3 sphereScale = { 0.1f, 0.1f, 0.1f };

	void Application::Run()
	{
		m_ImguiLayer->Init();


		m_Camera = std::make_shared<Rose::PerspectiveCameraController>(glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 10000.0f));
		m_Camera->GetCam().SetPosition({ 0.0f, 0.0f, -10.0f });
		UniformBufferData ubo;
		


		ubo.Model = glm::mat4(1.0f);
		while (!glfwWindowShouldClose(m_Window))
		{
			m_Camera->OnUpdate(0.016f);

			if (useCamera)
			{
				ubo.View = m_Camera->GetCam().GetView();
				ubo.Proj = m_Camera->GetCam().GetProj();
				ubo.ViewProj = m_Camera->GetCam().GetProjView();
			} else {
				ubo.View = glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, -10.0f });
				ubo.Proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
				ubo.ViewProj = ubo.Proj * ubo.View;

			}


			glfwPollEvents();
			for (auto& mat : m_TestModel->GetMaterials())
			{
				mat.ShaderData->UpdateUniformBuffer(&ubo, sizeof(ubo), 0);
			}

			for (auto& mat : m_SphereModel->GetMaterials())
			{
				mat.ShaderData->UpdateUniformBuffer(&ubo, sizeof(ubo), 0);
			}


			m_SkyboxShader->UpdateUniformBuffer(&ubo, sizeof(ubo), 0);
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

	}

	void Application::CreateFramebuffers()
	{
		m_Framebuffers.resize(m_SwapChain->GetImageViews().size());

		for (size_t i = 0; i < m_SwapChain->GetImageViews().size(); i++) {
			std::array<VkImageView,3> attachments = {
				m_SwapChain->GetMultisampledColorImageView(),
				m_SwapChain->GetDepthImageView(),
				m_SwapChain->GetImageViews()[i],
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_TestModel->GetMaterials()[0].ShaderData->GetRenderPass();
			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = attachments.data();
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
			auto& verticies = mesh.Verticies;

			float size = sizeof(Vertex) * verticies.size();
			m_VBOs.push_back(std::make_shared<Rose::VertexBuffer>(verticies.data(), size));
		}

		for (auto& mesh : m_SphereModel->GetMeshes())
		{
			auto& verticies = mesh.Verticies;

			float size = sizeof(Vertex) * verticies.size();
			m_SphereVbo.push_back(std::make_shared<Rose::VertexBuffer>(verticies.data(), size));
		}


	}

	void Application::CreateIndexBuffer()
	{

		for (auto& mesh : m_TestModel->GetMeshes())
		{
			auto& indicies = mesh.Indicies;
			m_IBOs.push_back(std::make_shared<Rose::IndexBuffer>(indicies.data(), sizeof(indicies[0]) * indicies.size()));
		}
		for (auto& mesh : m_SphereModel->GetMeshes())
		{
			auto& indicies = mesh.Indicies;
			m_SphereIbo.push_back(std::make_shared<Rose::IndexBuffer>(indicies.data(), sizeof(indicies[0]) * indicies.size()));
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
		renderPassInfo.renderPass = m_TestModel->GetMaterials()[0].ShaderData->GetRenderPass();
		renderPassInfo.framebuffer = m_Framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetExtent2D();


		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.5f, 0.1f, 0.1f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_VKCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		{
			VkBuffer vbos[] = { m_SkyboxVbo->GetBufferID() };
			VkDeviceSize offset[] = { 0 };

			const auto& shader = m_SkyboxShader;

			vkCmdBindPipeline(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetGrahpicsPipeline());

			vkCmdBindVertexBuffers(m_VKCommandBuffer, 0, 1, vbos, offset);
			vkCmdBindIndexBuffer(m_VKCommandBuffer, m_SkyboxIbo->GetBufferID(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipelineLayout(), 0, 1, &shader->GetDescriptorSet(), 0, nullptr);
			vkCmdDrawIndexed(m_VKCommandBuffer, 36, 1, 0, 0, 0);
			//vkCmdDraw(m_VKCommandBuffer, 36, 1, 0, 0);
		}


		for (int i = 0; i < m_VBOs.size(); i++)
		{

			VkBuffer vbos[] = { m_VBOs[i]->GetBufferID() };
			VkDeviceSize offset[] = { 0 };

			const auto& shader = m_TestModel->GetMaterials()[i].ShaderData;

			vkCmdBindPipeline(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetGrahpicsPipeline());

			vkCmdBindVertexBuffers(m_VKCommandBuffer, 0, 1, vbos, offset);
			vkCmdBindIndexBuffer(m_VKCommandBuffer, m_IBOs[i]->GetBufferID(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipelineLayout(), 0, 1, &shader->GetDescriptorSet(), 0, nullptr);
			vkCmdDrawIndexed(m_VKCommandBuffer, m_TestModel->GetMeshes()[i].Indicies.size(), 1, 0, 0, 0);

		}
		for (int i = 0; i < m_SphereVbo.size(); i++)
		{

			VkBuffer vbos[] = { m_SphereVbo[i]->GetBufferID() };
			VkDeviceSize offset[] = { 0 };

			const auto& shader = m_SphereModel->GetMaterials()[i].ShaderData;

			vkCmdBindPipeline(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetGrahpicsPipeline());

			vkCmdBindVertexBuffers(m_VKCommandBuffer, 0, 1, vbos, offset);
			vkCmdBindIndexBuffer(m_VKCommandBuffer, m_SphereIbo[i]->GetBufferID(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(m_VKCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipelineLayout(), 0, 1, &shader->GetDescriptorSet(), 0, nullptr);
			vkCmdDrawIndexed(m_VKCommandBuffer, m_SphereModel->GetMeshes()[i].Indicies.size(), 1, 0, 0, 0);

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

		for (auto& vbo : m_SphereVbo)
		{
			vbo->FreeMemory();
		}
		for (auto& ibo : m_SphereIbo)
		{
			ibo->FreeMemory();
		}

		m_SkyboxVbo->FreeMemory();
		//m_SkyboxIbo->FreeMemory();



		for (auto fb : m_Framebuffers) {
			vkDestroyFramebuffer(m_RenderingContext->GetLogicalDevice()->GetDevice(), fb, nullptr);
		}

		m_SwapChain->Destroy();


		m_ImguiLayer->Shutdown();
		m_TestModel->CleanUp();

		m_SkyboxShader->DestroyPipeline();
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
		ImGui::SliderFloat3("model pos", &spherePos.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("model scale", &sphereScale.x, -10.0f, 10.0f);
		ImGui::Checkbox("Use camera", &useCamera);
		ImGui::End();
	}

}