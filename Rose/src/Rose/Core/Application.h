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
	struct VertexData
	{
		glm::vec3 Position;

		static VkVertexInputBindingDescription GetBindingDescription() {

			VkVertexInputBindingDescription result{};
			result.binding = 0;
			result.stride = sizeof(VertexData);
			result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return result;
		}

		static std::array<VkVertexInputAttributeDescription, 1> GetAttributeDescription()
		{
			std::array<VkVertexInputAttributeDescription, 1> result{};

			result[0].binding = 0;
			result[0].location = 0;
			result[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			result[0].offset = offsetof(VertexData, Position);

// 
// 			result[1].binding = 0;
// 			result[1].location = 1;
// 			result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
// 			result[1].offset = offsetof(VertexData, Color);
// 
// 			result[2].binding = 0;
// 			result[2].location = 2;
// 			result[2].format = VK_FORMAT_R32G32_SFLOAT;
// 			result[2].offset = offsetof(VertexData, TexCoord);

			return result;

		}
	};

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

			std::shared_ptr<Shader>& GetShader() { return m_Shader; }
			std::shared_ptr<Texture2D>& GetTexture() { return m_Texture; }

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


			std::vector<Rose::VertexData> m_VertexData;
			std::vector<uint32_t> m_IndexData;

			std::vector<std::shared_ptr<Rose::VertexBuffer>> m_VBOs;
			std::vector<std::shared_ptr<Rose::IndexBuffer>> m_IBOs;

			std::shared_ptr<Rose::Texture2D> m_Texture;

			std::shared_ptr<Rose::PerspectiveCameraController> m_Camera;


			std::vector<VkFramebuffer> m_Framebuffers;

			VkCommandBuffer m_VKCommandBuffer;

			std::shared_ptr<Shader> m_Shader;
			std::shared_ptr<Model> m_TestModel;


			static Application* s_INSTANCE;

			

	};
}