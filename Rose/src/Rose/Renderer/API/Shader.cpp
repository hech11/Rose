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

		CreateUniformBuffer();
		CreateDiscriptorSetLayout();
		CreateDescriptorPool();
		CreateDescriptorSets();

		CreateShaderStagePipeline();




		for (auto&& [type, module] : m_ShaderModules)
		{
			vkDestroyShaderModule(Application::Get().GetLogicalDevice(), module, nullptr);
		}

	}

	Shader::~Shader()
	{
	}

	void Shader::DestroyPipeline()
	{

		vkDestroyBuffer(Application::Get().GetLogicalDevice(), m_UniformBuffer, nullptr);
		vkFreeMemory(Application::Get().GetLogicalDevice(), m_UBDeviceMemory, nullptr);

		vkDestroyDescriptorSetLayout(Application::Get().GetLogicalDevice(), m_DescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(Application::Get().GetLogicalDevice(), m_DescriptorPool, nullptr);



		vkDestroyPipeline(Application::Get().GetLogicalDevice(), m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(Application::Get().GetLogicalDevice(), m_PipelineLayout, nullptr);
		vkDestroyRenderPass(Application::Get().GetLogicalDevice(), m_RenderPass, nullptr);
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
				shaderSources[currentShaderType].first = currentShaderType;
				shaderSources[currentShaderType].second << line << "\n";
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

	}

	VkShaderModule Shader::CreateModule(const std::vector<uint32_t>& sprvCode)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = sprvCode.size()*sizeof(uint32_t);
		createInfo.pCode = sprvCode.data();

		VkShaderModule result;
		vkCreateShaderModule(Application::Get().GetLogicalDevice(), &createInfo, nullptr, &result);


		return result;
	}

	void Shader::CreateShaderStagePipeline()
	{
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
		viewport.y = 0.0f;
		viewport.width = (float)Application::Get().GetSwapChainExtent().width;
		viewport.height = (float)Application::Get().GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = Application::Get().GetSwapChainExtent();

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
		//rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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

		vkCreatePipelineLayout(Application::Get().GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
		
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = Application::Get().GetSwapChainImageFormat();
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


		vkCreateRenderPass(Application::Get().GetLogicalDevice(), &renderPassInfo, nullptr, &m_RenderPass);



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

		vkCreateGraphicsPipelines(Application::Get().GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline);
	}

	

	void Shader::CreateDiscriptorSetLayout()
	{

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		vkCreateDescriptorSetLayout(Application::Get().GetLogicalDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout);
	}

	void Shader::CreateUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferData);


		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		const auto& VKLogicalDevice = Application::Get().GetLogicalDevice();

		vkCreateBuffer(VKLogicalDevice, &bufferInfo, nullptr, &m_UniformBuffer);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(VKLogicalDevice, m_UniformBuffer, &memRequirements);


		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(Application::Get().GetPhysicalDevice(), &memProperties);

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

		if (vkAllocateMemory(VKLogicalDevice, &allocInfo, nullptr, &m_UBDeviceMemory) != VK_SUCCESS)
			ASSERT();

		vkBindBufferMemory(Application::Get().GetLogicalDevice(), m_UniformBuffer, m_UBDeviceMemory, 0);
	}

	void Shader::UpdateUniformBuffer(UniformBufferData& ubo)
	{
		void* data;
		vkMapMemory(Application::Get().GetLogicalDevice(), m_UBDeviceMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(Application::Get().GetLogicalDevice(), m_UBDeviceMemory);

	}

	void Shader::CreateDescriptorPool()
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;

		poolInfo.maxSets = 1;

		vkCreateDescriptorPool(Application::Get().GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool);
	}

	void Shader::CreateDescriptorSets()
	{

		std::vector<VkDescriptorSetLayout> layouts(1, m_DescriptorSetLayout); // TODO: multiple frames in flight
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts.data();
		vkAllocateDescriptorSets(Application::Get().GetLogicalDevice(), &allocInfo, &m_DescriptorSet);


		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferData);

		VkWriteDescriptorSet descWrite{};
		descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite.dstSet = m_DescriptorSet;
		descWrite.dstBinding = 0;
		descWrite.dstArrayElement = 0;
		descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descWrite.descriptorCount = 1;

		descWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(Application::Get().GetLogicalDevice(), 1, &descWrite, 0, nullptr);

	}

	void Shader::Reflect()
	{

	}

}