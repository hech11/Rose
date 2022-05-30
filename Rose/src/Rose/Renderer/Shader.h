#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>

namespace Rose
{
	enum class ShaderMemberType
	{
		None = -1,
		Int8,
		Int16,
		Int32,
		UInt32,
		Float,
		Bool,
		Mat4,
		SampledImage
	};

	struct ShaderMember
	{
		std::string Name;
		uint32_t Size;
		uint32_t Offset;
		ShaderMemberType Type;

	};

	struct ShaderUniformBuffer
	{
		uint32_t BufferSize;
		uint32_t Binding;
		uint32_t MemberSize;
		std::vector<ShaderMember> Members;
	};

	struct ShaderResource
	{
		std::string Name;
		std::vector<ShaderUniformBuffer> ReflectedUBOs;
	};

	enum ShaderModuleTypes
	{
		Vertex, Pixel
	};

	class Shader
	{

		public :
			Shader(const std::string& filepath);
			~Shader();

			void DestroyPipeline();

		private :
			void ParseShaders(const std::string& filepath);

			void CompileShadersIntoSPIRV();
			VkShaderModule CreateModule(const std::vector<uint32_t>& sprvCode);

			void CreateShaderStagePipeline();
			void CreateRenderPass();

			void Reflect();

		private :
			std::string m_Filepath;
			std::string m_Name;

			std::unordered_map<ShaderModuleTypes, std::string> m_UncompiledShaderSources;
			std::unordered_map <ShaderModuleTypes, std::vector<uint32_t>> m_CompiledShaderSources;
			std::unordered_map <ShaderModuleTypes, VkShaderModule> m_ShaderModules;

			VkPipelineLayout m_PipelineLayout;
			VkRenderPass m_RenderPass;

	};

}