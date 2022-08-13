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


	}


	Shader::Shader(const std::string& filepath, const ShaderAttributeLayout& layout)
		: m_Filepath(filepath), m_AttributeLayout(layout)
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

		

		auto bindingDesc = m_AttributeLayout.BindingDescription;
		auto attributeDesc = m_AttributeLayout.ReturnVKAttribues();
		

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
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = Application::Get().GetContext()->GetPhysicalDevice()->GetMSAASampleCount();
		multisampling.minSampleShading = 0.5f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_TRUE;
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
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;


		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;

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
		colorAttachment.samples = Application::Get().GetContext()->GetPhysicalDevice()->GetMSAASampleCount();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = Application::Get().GetContext()->GetPhysicalDevice()->FindDepthFormat();
		depthAttachment.samples = Application::Get().GetContext()->GetPhysicalDevice()->GetMSAASampleCount();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = Application::Get().GetSwapChain()->GetColorFormat();
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass);



		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline);
	}

	

	void Shader::CreateDiscriptorSetLayout()
	{

		const VkDevice& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();
		std::vector<VkDescriptorSetLayoutBinding> bindings;


		for (auto& resource : m_Resources)
		{
			if (resource.UniformBufferSize)
			{

				auto stageFlag = Utils::DeduceShaderStageFromType(resource.Type);
				for (auto& ubo : resource.ReflectedUBOs)
				{
					VkDescriptorSetLayoutBinding binding{};

					binding.binding = ubo.Binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					binding.pImmutableSamplers = nullptr;
					binding.stageFlags = stageFlag;

					bindings.push_back(binding);
				}

			}

			if (resource.ImageBufferSize)
			{
				auto stageFlag = Utils::DeduceShaderStageFromType(resource.Type);
				for (auto& image : resource.ReflectedMembers)
				{
					if (image.Type == ShaderMemberType::SampledImage)
					{
						VkDescriptorSetLayoutBinding binding{};
						binding.binding = image.Binding;
						binding.descriptorCount = 1;
						binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						binding.pImmutableSamplers = nullptr;
						binding.stageFlags = stageFlag;
						bindings.push_back(binding);

					}

				}
			}
		}


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


	void Shader::UpdateUniformBuffer(void* data, uint32_t size, uint32_t binding)
	{
		const auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		if (binding < 0 || binding > m_UniformBuffers.size())
		{
			LOG("Could not update UBO at binding '%d'! It may not exist or is 0!\n");
			return;
		}
		auto& memory = m_UniformBuffers[binding].DeviceMemory;


		void* temp;
		vkMapMemory(device, memory, 0, size, 0, &temp);
		memcpy(temp, data, size);
		vkUnmapMemory(device, memory);

	}

	void Shader::CreateDescriptorPool(const std::vector< MaterialUniform>& matUniforms)
	{


		const auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		std::vector<VkDescriptorPoolSize> poolSizes{};

		for (auto& resource : m_Resources)
		{
			if (resource.UniformBufferSize)
			{
				for (auto& ubo : resource.ReflectedUBOs)
				{
					VkDescriptorPoolSize result;
					result.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					result.descriptorCount = 1;

					poolSizes.push_back(result);
				}
			}

			if (resource.ImageBufferSize)
			{
				for (auto& image : resource.ReflectedMembers)
				{
					if (image.Type == ShaderMemberType::SampledImage)
					{
						VkDescriptorPoolSize result;
						result.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						result.descriptorCount = 1;

						poolSizes.push_back(result);
					}
				}
			}

		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();

		poolInfo.maxSets = 1;

		vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);
	}

	void Shader::CreateDescriptorSets(const std::vector< MaterialUniform>& matUniforms)
	{
		const auto& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		std::vector<VkDescriptorSetLayout> layouts(1, m_DescriptorSetLayout); // TODO: multiple frames in flight
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts.data();
		VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet);


		std::vector<VkWriteDescriptorSet> descWrites{};
		std::vector<VkDescriptorImageInfo> imgInfos{};

		for (auto& resource : m_Resources)
		{
			if (resource.UniformBufferSize)
			{
				
				for (auto& ubo : resource.ReflectedUBOs)
				{

					VkDescriptorBufferInfo bufferInfo{};

					bufferInfo.buffer = m_UniformBuffers[ubo.Binding].Buffer;
					bufferInfo.range = ubo.BufferSize;
					bufferInfo.offset = 0;

					VkWriteDescriptorSet bufferDescWrite{};

					bufferDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					bufferDescWrite.dstSet = m_DescriptorSet;
					bufferDescWrite.dstBinding = ubo.Binding;
					bufferDescWrite.dstArrayElement = 0;
					bufferDescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					bufferDescWrite.descriptorCount = 1;
					bufferDescWrite.pBufferInfo = &bufferInfo;
					

					descWrites.push_back(bufferDescWrite);
				}
			}

			if (resource.ImageBufferSize)
			{

				int i = 0;
				imgInfos.resize(resource.ImageBufferSize);

				for (auto& image : resource.ReflectedMembers)
				{
					if (image.Type == ShaderMemberType::SampledImage)
					{

					
						if (matUniforms.size())
						{
							if (i < matUniforms.size())
							{
								imgInfos[i].imageView = matUniforms[i].Texture->GetImageView();
								imgInfos[i].sampler = matUniforms[i].Texture->GetSampler();
							}
							LOG("%d\n", i);
						}
						else {
							imgInfos[i].imageView = Material::DefaultWhiteTexture()->GetImageView();
							imgInfos[i].sampler = Material::DefaultWhiteTexture()->GetSampler();
						}

						if (i < matUniforms.size())
						{
							imgInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						}

						VkWriteDescriptorSet imageDescWrite{};

						imageDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						imageDescWrite.dstSet = m_DescriptorSet;
						imageDescWrite.dstBinding = image.Binding;
						imageDescWrite.dstArrayElement = 0;
						imageDescWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						imageDescWrite.descriptorCount = 1;
						imageDescWrite.pImageInfo = &imgInfos[i];

						descWrites.push_back(imageDescWrite);
						i++;


					}
				}
			}

		}


		vkUpdateDescriptorSets(device, descWrites.size(), descWrites.data(), 0, nullptr);

	}

	void Shader::CreatePipelineAndDescriptorPool(const std::vector< MaterialUniform>& matUniforms)
	{
		CreateDescriptorPool(matUniforms);
		CreateDescriptorSets(matUniforms);

		CreateShaderStagePipeline();



		const VkDevice& device = Application::Get().GetContext()->GetLogicalDevice()->GetDevice();

		for (auto&& [type, module] : m_ShaderModules)
		{
			vkDestroyShaderModule(device, module, nullptr);
		}

	}

	void Shader::Reflect(ShaderModuleTypes type, const std::vector<uint32_t>& data, bool logInfo)
	{
		spirv_cross::Compiler compiler(data);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		ShaderResource shaderResource;
		shaderResource.Name = std::string(m_Name);
		shaderResource.Type = type;
		shaderResource.UniformBufferSize = resources.uniform_buffers.size();
		shaderResource.ImageBufferSize = resources.sampled_images.size();

		for (uint32_t i = 0; i < resources.sampled_images.size(); i++)
		{
			auto& image = resources.sampled_images[i];

			
			ShaderMember member;
			auto& id = compiler.get_type(image.type_id);
			member.Name = compiler.get_member_name(image.base_type_id, i).c_str();
			member.Type = Utils::FromSpirVTypeToShaderMemberType(id);


			member.Size = ShaderMember::ShaderTypeToBytes(member.Type);
			member.Binding = compiler.get_decoration(image.id, spv::Decoration::DecorationBinding);
			member.Offset = 0;

			shaderResource.ReflectedMembers.push_back(member); // What about general uniforms?
		}


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

				member.Size = ShaderMember::ShaderTypeToBytes(member.Type);
				offset += member.Size;

				uniform.Members.push_back(member);

			}

			shaderResource.ReflectedUBOs.push_back(uniform);

		}

		if (logInfo)
		{

			LOG("\n\nShader resource: (%s::%s)\n", shaderResource.Name.c_str(), Utils::FromShaderTypeToString(shaderResource.Type).c_str());
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
