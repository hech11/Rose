#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include <glm/glm.hpp>

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

	struct UniformBufferData
	{
		glm::mat4 Model, ViewProj;
	};

	class Shader
	{

		public :
			Shader(const std::string& filepath);
			~Shader();

			void DestroyPipeline();
			void CreateUniformBuffer();


			void UpdateUniformBuffer(UniformBufferData& ubo);


			const VkPipeline& GetGrahpicsPipeline() const { return m_GraphicsPipeline; }
			VkPipeline& GetGrahpicsPipeline() { return m_GraphicsPipeline; }

			VkPipelineLayout& GetPipelineLayout() { return m_PipelineLayout; }
			const VkPipelineLayout& GetPipelineLayout() const { return m_PipelineLayout; }

			const VkRenderPass& GetRenderPass() const { return m_RenderPass; }
			VkRenderPass& GetRenderPass() { return m_RenderPass; }

			VkDescriptorSet& GetDescriptorSet() { return m_DescriptorSet; }
			const VkDescriptorSet& GetDescriptorSet() const { return m_DescriptorSet; }

			void CreateDiscriptorSetLayout();

			void CreateDescriptorPool();
			void CreateDescriptorSets();

		private :
			void ParseShaders(const std::string& filepath);

			void CompileShadersIntoSPIRV();
			VkShaderModule CreateModule(const std::vector<uint32_t>& sprvCode);

			void CreateShaderStagePipeline();
			void Reflect();

		private :
			std::string m_Filepath;
			std::string m_Name;

			std::unordered_map<ShaderModuleTypes, std::string> m_UncompiledShaderSources;
			std::unordered_map <ShaderModuleTypes, std::vector<uint32_t>> m_CompiledShaderSources;
			std::unordered_map <ShaderModuleTypes, VkShaderModule> m_ShaderModules;

			VkPipeline m_GraphicsPipeline;
			VkDescriptorSetLayout m_DescriptorSetLayout;
			VkPipelineLayout m_PipelineLayout;
			VkRenderPass m_RenderPass;

			VkBuffer m_UniformBuffer;
			VkDeviceMemory m_UBDeviceMemory;

			VkDescriptorPool m_DescriptorPool;
			VkDescriptorSet m_DescriptorSet;


	};

}