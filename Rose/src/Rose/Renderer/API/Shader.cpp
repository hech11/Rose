#include "Shader.h"
#include "Rose\Core\Log.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <vulkan1.2.182.0/include/Vulkan/spirv_cross/spirv_glsl.hpp>
#include <vulkan1.2.182.0/include/Vulkan/spirv_cross/spirv_reflect.hpp>
#include <vulkan1.2.182.0/include/Vulkan/spirv_cross/spirv_cross.hpp>
#include <vulkan1.2.182.0/include/Vulkan/shaderc/shaderc.hpp>

#include <tuple>
#include "Rose/Core/Application.h"


namespace Rose
{


	namespace Utils {
	
		static shaderc_shader_kind FromVKShaderTypeToSPIRV(ShaderModuleTypes type)
		{
			switch (type)
			{
			case Rose::ShaderModuleTypes::Vertex: return shaderc_glsl_vertex_shader;
			case Rose::ShaderModuleTypes::Pixel: return shaderc_glsl_fragment_shader;
			}
		}

		static VkShaderStageFlagBits DeduceShaderStageFromType(ShaderModuleTypes type)
		{
			switch (type)
			{
			case Rose::ShaderModuleTypes::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
			case Rose::ShaderModuleTypes::Pixel:return VK_SHADER_STAGE_FRAGMENT_BIT;
			}
		}

		static std::string FromShaderTypeToString(ShaderModuleTypes type)
		{
			switch (type)
			{
				case Rose::ShaderModuleTypes::Vertex: return "vertex";
				case Rose::ShaderModuleTypes::Pixel: return "pixel";
			}
		}

		
		static std::string FromShaderMemberTypeToString(ShaderMemberType type)
		{
			switch (type)
			{
				case Rose::ShaderMemberType::None: return "None?";
				case Rose::ShaderMemberType::Int8: return "Int8";
				case Rose::ShaderMemberType::Int16: return "Int16";
				case Rose::ShaderMemberType::UInt32: return "UInt32";
				case Rose::ShaderMemberType::Float: return "Float";
				case Rose::ShaderMemberType::Bool: return "Bool";
				case Rose::ShaderMemberType::Mat4: return "Mat4";
				case Rose::ShaderMemberType::SampledImage: return "SampledImage";
				default: return "Not listed!";
			}
		}

		static ShaderMemberType FromSpirVTypeToShaderMemberType(const spirv_cross::SPIRType& type)
		{
			switch (type.basetype)
			{
				case spirv_cross::SPIRType::BaseType::Char: return ShaderMemberType::Int8;
				case spirv_cross::SPIRType::BaseType::Short: return ShaderMemberType::Int16;
				case spirv_cross::SPIRType::BaseType::Int: return ShaderMemberType::Int32;
				case spirv_cross::SPIRType::BaseType::Float:
				{
					if (type.columns == 4)
						return ShaderMemberType::Mat4;

					return ShaderMemberType::Float;
				}
				case spirv_cross::SPIRType::BaseType::SampledImage: return ShaderMemberType::SampledImage;
			}
		}

		static uint32_t ShaderTypeToBytes(const ShaderMemberType& type)
		{
			switch (type)
			{
				case ShaderMemberType::Bool: return 1;
				case ShaderMemberType::Float: return 4;
				case ShaderMemberType::Int16: return 2;
				case ShaderMemberType::Int32: return 4;
				case ShaderMemberType::Mat4: return 4 * 4 * 4;
			}
		}

	}


	Shader::Shader(const std::string& filepath) 
		: m_Filepath(filepath)
	{
		m_Name = std::filesystem::path(m_Filepath).stem().string();
		ParseShaders(m_Filepath);

		CompileShadersIntoSPIRV();
		for (auto&& [type, compiledSources] : m_CompiledShaderSources)
		{
			m_ShaderModules[type] = CreateModule(compiledSources);
		}

		CreateUniformBuffers();
		CreateDiscriptorSetLayout();
		CreateDescriptorPool();
		CreateDescriptorSets();

		CreateShaderStagePipeline();



		const VkDevice& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		for (auto&& [type, module] : m_ShaderModules)
		{
			vkDestroyShaderModule(device, module, nullptr);
		}

	}

	Shader::~Shader()
	{
	}

	void Shader::DestroyPipeline()
	{


		const VkDevice& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		for (auto& ubo : m_UniformBuffers)
		{
			vkDestroyBuffer(device, ubo.Buffer, nullptr);
			vkFreeMemory(device, ubo.DeviceMemory, nullptr);
		}


		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);



		vkDestroyPipeline(device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(device, m_RenderPass, nullptr);
	}

	void Shader::ParseShaders(const std::string& filepath)
	{

		std::ifstream file(filepath, std::ios::in, std::ios::binary);
	
		std::vector<std::pair<ShaderModuleTypes, std::stringstream>> shaderSources(2);
		
		ShaderModuleTypes currentShaderType = ShaderModuleTypes::Vertex;
		std::string line;

		if (!file.is_open())
		{
			ASSERT();
		}

		while (std::getline(file, line))
		{
			if (line.find("#type") != std::string::npos)
			{
				if (line.find("vertex") != std::string::npos)
				{
					currentShaderType = ShaderModuleTypes::Vertex;
				}
				else if (line.find("pixel") != std::string::npos)
				{
					currentShaderType = ShaderModuleTypes::Pixel;
				}
			}
			else
			{
				shaderSources[(int)currentShaderType].first = currentShaderType;
				shaderSources[(int)currentShaderType].second << line << "\n";
			}
		}
		for (auto& sources : shaderSources)
		{
			const auto& srcType = sources.first;
			const auto& source  = sources.second;

			m_UncompiledShaderSources[srcType] = source.str();

		}
	}



	void Shader::CompileShadersIntoSPIRV()
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		
		for (auto&& [type, source] : m_UncompiledShaderSources)
		{


			shaderc::SpvCompilationResult results = compiler.CompileGlslToSpv(source, Utils::FromVKShaderTypeToSPIRV(type), m_Name.c_str());

			if (results.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				LOG("%s\n", results.GetErrorMessage().c_str());
			}

			m_CompiledShaderSources[type] = std::vector<uint32_t>(results.cbegin(), results.cend());
		}

		for (auto& [type, data] : m_CompiledShaderSources)
		{
			Reflect(type, data, true);
		}

	}

	VkShaderModule Shader::CreateModule(const std::vector<uint32_t>& sprvCode)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = sprvCode.size()*sizeof(uint32_t);
		createInfo.pCode = sprvCode.data();

		VkShaderModule result;

		const VkDevice& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();
		vkCreateShaderModule(device, &createInfo, nullptr, &result);



		return result;
	}

	void Shader::CreateShaderStagePipeline()
	{

		const VkDevice& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (auto&& [type, module] : m_ShaderModules)
		{
			VkPipelineShaderStageCreateInfo stageInfo{};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = Utils::DeduceShaderStageFromType(type);
			stageInfo.pName = "main";
			stageInfo.module = module;

			shaderStages.push_back(stageInfo);
		}

		auto bindingDesc = VertexData::GetBindingDescription();
		auto attributeDesc = VertexData::GetAttributeDescription();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDesc.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data(); 

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;


		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = (float)Application::Get().GetSwapChain()->GetExtent2D().height;
		viewport.width = (float)Application::Get().GetSwapChain()->GetExtent2D().width;
		viewport.height = -(float)Application::Get().GetSwapChain()->GetExtent2D().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = Application::Get().GetSwapChain()->GetExtent2D();

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;


		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		//rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();


		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;

		vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
		
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = Application::Get().GetSwapChain()->GetColorFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;


		vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass);



		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.data();

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline);
	}

	

	void Shader::CreateDiscriptorSetLayout()
	{

		const VkDevice& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout);
	}

	void Shader::CreateUniformBuffers()
	{
		for (auto& resources : m_Resources)
		{
			int i = 0;
			for (auto& reflectedUBO : resources.ReflectedUBOs)
			{
				VkBuffer buffer;
				VkDeviceMemory deviceMemory;


				VkBufferCreateInfo bufferInfo{};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = reflectedUBO.BufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				const auto& VKLogicalDevice = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

				vkCreateBuffer(VKLogicalDevice, &bufferInfo, nullptr, &buffer);


				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements(VKLogicalDevice, buffer, &memRequirements);


				VkPhysicalDeviceMemoryProperties memProperties;
				vkGetPhysicalDeviceMemoryProperties(Application::Get().GetContext()->GetPhysicalDevice()->GetDevice(), &memProperties);

				uint32_t memTypeIndex = 0;
				uint32_t propFilter = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
					if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & propFilter) == propFilter)
					{
						memTypeIndex = i;
						break;
					}
				}

				VkMemoryAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = memTypeIndex;

				if (vkAllocateMemory(VKLogicalDevice, &allocInfo, nullptr, &deviceMemory) != VK_SUCCESS)
					ASSERT();

				vkBindBufferMemory(VKLogicalDevice, buffer, deviceMemory, 0);

				i++;

				m_UniformBuffers.push_back({ buffer, deviceMemory });
			}
		}
		

	}

	void Shader::UpdateUniformBuffer(UniformBufferData& ubo)
	{
		const auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();


		void* data;
		vkMapMemory(device, m_UBDeviceMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, m_UBDeviceMemory);

	}

	void Shader::CreateDescriptorPool()
	{


		const auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		std::array<VkDescriptorPoolSize, 1> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();

		poolInfo.maxSets = 1;

		vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);
	}

	void Shader::CreateDescriptorSets()
	{
		const auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		std::vector<VkDescriptorSetLayout> layouts(1, m_DescriptorSetLayout); // TODO: multiple frames in flight
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts.data();
		VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet);


		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferData);

		VkDescriptorImageInfo imageInfo{};
		
		imageInfo.imageView = Application::Get().GetTexture()->GetImageView();
		imageInfo.sampler = Application::Get().GetTexture()->GetSampler();
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;




		std::array<VkWriteDescriptorSet, 1> descWrites{};

		descWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrites[0].dstSet = m_DescriptorSet;
		descWrites[0].dstBinding = 0;
		descWrites[0].dstArrayElement = 0;
		descWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descWrites[0].descriptorCount = 1;
		descWrites[0].pBufferInfo = &bufferInfo;

// 		descWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
// 		descWrites[1].dstSet = m_DescriptorSet;
// 		descWrites[1].dstBinding = 1;
// 		descWrites[1].dstArrayElement = 0;
// 		descWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
// 		descWrites[1].descriptorCount = 1;
// 		descWrites[1].pImageInfo = &imageInfo;
// 

		vkUpdateDescriptorSets(device, descWrites.size(), descWrites.data(), 0, nullptr);

	}

	void Shader::Reflect(ShaderModuleTypes type, const std::vector<uint32_t>& data, bool logInfo)
	{
		spirv_cross::Compiler compiler(data);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		ShaderResource shaderResource;
		shaderResource.Name = std::string(m_Name) + "::" + Utils::FromShaderTypeToString(type).c_str();
		shaderResource.UniformBufferSize = resources.uniform_buffers.size();
		shaderResource.ImageBufferSize = resources.sampled_images.size();



		for (auto& resource : resources.uniform_buffers)
		{

			ShaderUniformBuffer uniform;
			auto& id = compiler.get_type(resource.type_id);
			uniform.BufferSize = compiler.get_declared_struct_size(id);
			uniform.Binding = compiler.get_decoration(resource.id, spv::Decoration::DecorationBinding);
			uniform.MemberSize = id.member_types.size();


			uint32_t offset = 0;
			for (int i = 0; i < uniform.MemberSize; i++)
			{

				ShaderMember member;
				member.Name = compiler.get_member_name(resource.base_type_id, i).c_str();
				member.Type = Utils::FromSpirVTypeToShaderMemberType(compiler.get_type(id.member_types[i]));


				member.Offset = offset;

				member.Size = Utils::ShaderTypeToBytes(member.Type);
				offset += member.Size;

				uniform.Members.push_back(member);

			}

			shaderResource.ReflectedUBOs.push_back(uniform);

		}

		if (logInfo)
		{

			LOG("\n\nShader resource: (%s)\n", shaderResource.Name.c_str());
			LOG("Uniform buffers: (%d)\n", shaderResource.UniformBufferSize);
			LOG("Image buffers: (%d)\n", shaderResource.ImageBufferSize);
			LOG("Reflected UBOs: (count: %d)\n", shaderResource.ReflectedUBOs.size());
			for (auto& ubo : shaderResource.ReflectedUBOs)
			{
				LOG("\t--Binding: (%d)\n", ubo.Binding);
				LOG("\t--Buffer size: (%d)\n", ubo.BufferSize);


				LOG("\t--Members: (count %d)\n", ubo.Members.size());
				for (auto& member : ubo.Members)
				{
					LOG("\t\t--Member: %s\n", member.Name.c_str());
					LOG("\t\t\t--Type: %s\n", Utils::FromShaderMemberTypeToString(member.Type).c_str());
					LOG("\t\t\t--Size: %d\n", member.Size);
					LOG("\t\t\t--Offset: %d\n", member.Offset);
				}
				LOG("\t---\n",);

			}
			LOG("---\n", );

		}

		m_Resources.push_back(shaderResource);

	}

}
