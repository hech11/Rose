#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <vulkan1.2.182.0/include/Vulkan/vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "Rose/Core/LOG.h"

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
		Float2,
		Float3,
		Float4,
		Bool,
		Mat4,
		SampledImage
	};


	enum class ShaderModuleTypes
	{
		Vertex, Pixel
	};


	struct ShaderMember
	{
		std::string Name;
		uint32_t Size;
		uint32_t Offset;
		int32_t Binding = -1;
		ShaderMemberType Type;



		static uint32_t ShaderTypeToBytes(const ShaderMemberType& type)
		{
			switch (type)
			{
				case ShaderMemberType::Bool: return 1;
				case ShaderMemberType::Int8: return 1;
				case ShaderMemberType::Int16: return 2;
				case ShaderMemberType::Int32: return 4;
				case ShaderMemberType::UInt32: return 4;
				case ShaderMemberType::Float: return 4;
				case ShaderMemberType::Float2: return 4 + 4;
				case ShaderMemberType::Float3: return 4 + 4 + 4;
				case ShaderMemberType::Float4: return 4 + 4 + 4 + 4;
				case ShaderMemberType::Mat4: return 4 * 4 * 4;
			}
		}
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
		ShaderModuleTypes Type;


		uint32_t UniformBufferSize;
		uint32_t ImageBufferSize;

		std::vector<ShaderUniformBuffer> ReflectedUBOs;
		std::vector<ShaderMember> ReflectedMembers;
	};

	struct UniformBufferData
	{
		glm::mat4 Model =  glm::mat4(1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::mat4 Proj = glm::mat4(1.0f);
		glm::mat4 ViewProj = glm::mat4(1.0f);
	};

	struct VKUniformBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory DeviceMemory;
	};


	struct ShaderAttribute 
	{
		std::string Name;
		uint32_t Location = 0;
		ShaderMemberType Format;

		uint32_t Size = 0;
		uint32_t Binding = 0;
		VkVertexInputAttributeDescription VKDescription;

		ShaderAttribute(const std::string& name, uint32_t location, ShaderMemberType format) 
			: Name(name), Location(location), Format(format)
		{
			Size = ShaderMember::ShaderTypeToBytes(Format);

			VKDescription.binding = Binding;
			VKDescription.location = Location;
			VKDescription.format = FromShaderMemberTypeToVKFormat(Format);

		}

		static VkFormat FromShaderMemberTypeToVKFormat(const ShaderMemberType& type)
		{
			switch (type)
			{
				default: ASSERT();
				case ShaderMemberType::None: ASSERT();
				case ShaderMemberType::Int8: return VK_FORMAT_R8_SINT;
				case ShaderMemberType::Int16: VK_FORMAT_R16_SINT;
				case ShaderMemberType::Int32:VK_FORMAT_R32_SINT;
				case ShaderMemberType::UInt32:return  VK_FORMAT_R32_UINT;
				case ShaderMemberType::Float: return  VK_FORMAT_R32_SFLOAT;
				case ShaderMemberType::Float2: return  VK_FORMAT_R32G32_SFLOAT;
				case ShaderMemberType::Float3: return  VK_FORMAT_R32G32B32_SFLOAT;
				case ShaderMemberType::Float4: return  VK_FORMAT_R32G32B32A32_SFLOAT;
				case ShaderMemberType::Bool: return VK_FORMAT_R8_UINT;
				case ShaderMemberType::Mat4: ASSERT();
				case ShaderMemberType::SampledImage: ASSERT();
			}
		}
	};


	struct ShaderAttributeLayout 
	{


		VkVertexInputBindingDescription BindingDescription{};

		
		ShaderAttributeLayout(const std::initializer_list<ShaderAttribute>& attributes)
			: Attributes(attributes)
		{

			uint32_t stride = 0;
			for (auto& attribute : Attributes)
			{
				attribute.VKDescription.offset = stride;
				stride += attribute.Size;
			}

			BindingDescription.binding = 0;
			BindingDescription.stride = stride;
			BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		std::vector<VkVertexInputAttributeDescription> ReturnVKAttribues()
		{
			std::vector< VkVertexInputAttributeDescription> result;

			for (auto& attribute : Attributes)
			{
				result.push_back(attribute.VKDescription);
			}
			return result;
		}

		std::vector<ShaderAttribute> Attributes;
	};



	struct MaterialUniform;
	class Shader
	{

		public :
			Shader(const std::string& filepath, const ShaderAttributeLayout& layout, bool isSkybox = false);
			~Shader();

			void DestroyPipeline();
			void CreateUniformBuffers();


			void UpdateUniformBuffer(void* data, uint32_t size, uint32_t binding);


			const VkPipeline& GetGrahpicsPipeline() const { return m_GraphicsPipeline; }
			VkPipeline& GetGrahpicsPipeline() { return m_GraphicsPipeline; }

			VkPipelineLayout& GetPipelineLayout() { return m_PipelineLayout; }
			const VkPipelineLayout& GetPipelineLayout() const { return m_PipelineLayout; }

			const VkRenderPass& GetRenderPass() const { return m_RenderPass; }
			VkRenderPass& GetRenderPass() { return m_RenderPass; }

			VkDescriptorSet& GetDescriptorSet() { return m_DescriptorSet; }
			const VkDescriptorSet& GetDescriptorSet() const { return m_DescriptorSet; }


			VkDescriptorPool& GetDescriptorPool() { return m_DescriptorPool; }

			void CreateDiscriptorSetLayout();

			void CreateDescriptorPool(const std::vector< MaterialUniform>& matUniforms);
			void CreateDescriptorSets(const std::vector< MaterialUniform>& matUniforms);


			void CreatePipelineAndDescriptorPool(const std::vector< MaterialUniform>& matUniforms);

		private :
			void ParseShaders(const std::string& filepath);

			void CompileShadersIntoSPIRV();
			VkShaderModule CreateModule(const std::vector<uint32_t>& sprvCode);

			void CreateShaderStagePipeline();
			void Reflect(ShaderModuleTypes type, const std::vector<uint32_t>& data, bool logInfo);

		private :
			std::string m_Filepath;
			std::string m_Name;

			std::unordered_map<ShaderModuleTypes, std::string> m_UncompiledShaderSources;
			std::unordered_map <ShaderModuleTypes, std::vector<uint32_t>> m_CompiledShaderSources;
			std::unordered_map <ShaderModuleTypes, VkShaderModule> m_ShaderModules;

			std::vector<ShaderResource> m_Resources;
			std::vector<VKUniformBuffer> m_UniformBuffers;


			VkPipeline m_GraphicsPipeline;
			VkDescriptorSetLayout m_DescriptorSetLayout;
			VkPipelineLayout m_PipelineLayout;
			VkRenderPass m_RenderPass;


			VkDescriptorPool m_DescriptorPool;
			VkDescriptorSet m_DescriptorSet;

			ShaderAttributeLayout m_AttributeLayout;
			bool m_IsSkybox = false; // TODO: Proper graphics pipelines needed!

	};

}