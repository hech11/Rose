#pragma once





#include <vector>
#include <array>
#include <optional>
#include <memory>
#include <glm/glm.hpp>

#include "Rose/Renderer/API/Shader.h"
#include "Rose/Renderer/API/VertexBuffer.h"
#include "Rose/Renderer/API/IndexBuffer.h"
#include "Rose/Renderer/RendererContext.h"
#include "Rose/Renderer/SwapChain.h"
#include "Rose/Renderer/API/Texture.h"
#include "Rose/Renderer/Model.h"

#include "Rose/Editor/ImguiLayer.h"

#include "Rose/Renderer/PerspectiveCamera.h"



namespace Rose
{
	
	enum class EventType
	{
		KeyPressed, KeyReleased,
		MouseMoved, MouseButtonClicked, MouseButtonReleased, MouseScrollWheelUsed
	};


	class Application
	{
		public :
			Application();
			~Application();


			void Run();


			void OnKeyPressedEvent(int key, int action);
			void OnKeyReleasedEvent(int key);
			void OnMouseMovedEvent(int button, int action);
			void OnMouseButtonClickedEvent(int button, int action);
			void OnMouseButtonReleasedEvent(int button);
			void OnMouseScrollWheelUsed(float x, float y);


			const GLFWwindow* GetWindow() const;
			GLFWwindow* GetWindow();

			VkSurfaceKHR GetWindowSurface() const { return m_SwapChain->GetWindowSurface(); }


			std::shared_ptr<SwapChain>& GetSwapChain() { return m_SwapChain; }

			const std::shared_ptr<RendererContext>& GetContext() const { return m_RenderingContext; }

			std::shared_ptr<Model>& GetTestModel() { return m_TestModel; }

			VkCommandBuffer& GetCommandBuffer() { return m_VKCommandBuffer; }

			std::vector<VkFramebuffer>& GetFramebuffers() { return m_Framebuffers; }

		public :

			static Application& Get() {
				return *s_INSTANCE;
			}


		private :
			void MakeWindow();
			void CreateVulkanInstance();


			void CreateGraphicsPipeline();
			void CreateFramebuffers();

			void CreateVertexBuffer();
			void CreateIndexBuffer();

			void CreateCommandPoolAndBuffer();

			void RecordCommandBuffer(uint32_t imageIndex);


			void DrawOntoScreen();


			void CreateWinGLFWSurface();

			void CleanUp();

			void OnImguiRender();

		private :
			GLFWwindow* m_Window = nullptr;
			ImguiLayer* m_ImguiLayer;

			std::shared_ptr<RendererContext> m_RenderingContext;
			std::shared_ptr<SwapChain> m_SwapChain;



			std::vector<std::shared_ptr<Rose::VertexBuffer>> m_VBOs;
			std::vector<std::shared_ptr<Rose::IndexBuffer>> m_IBOs;


			std::vector<std::shared_ptr<Rose::VertexBuffer>> m_SphereVbo;
			std::vector<std::shared_ptr<Rose::IndexBuffer>> m_SphereIbo;


			std::shared_ptr<Rose::PerspectiveCameraController> m_Camera;


			std::vector<VkFramebuffer> m_Framebuffers;

			VkCommandBuffer m_VKCommandBuffer;

			std::shared_ptr<Model> m_TestModel;
			std::shared_ptr<Model> m_SphereModel;


			static Application* s_INSTANCE;

			

	};
}