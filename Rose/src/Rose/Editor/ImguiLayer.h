#pragma once


struct ImGui_ImplVulkanH_Window;
namespace Rose
{

	class ImguiLayer
	{
		public :

			void Init();
			void Shutdown();

			void Begin();
			void End();

	};

}