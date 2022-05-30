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
				case Rose::Vertex: return shaderc_glsl_vertex_shader;
				case Rose::Pixel: return shaderc_glsl_fragment_shader;
			}
		}

		static VkShaderStageFlagBits DeduceShaderStageFromType(ShaderModuleTypes type)
		{
			switch (type)
			{
				case Rose::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
				case Rose::Pixel:return VK_SHADER_STAGE_FRAGMENT_BIT;
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

		CreateShaderStagePipeline();
		for (auto&& [type, module] : m_ShaderModules)
		{
			//vkDestroyShaderModule(, module, nullptr); //TODO: Create vk devide first!
		}

	}

	Shader::~Shader()
	{

	}

	void Shader::DestroyPipeline()
	{
		vkDestroyPipelineLayout(Application::Get().GetLogicalDevice(), m_PipelineLayout, nullptr);
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
		//options.SetOptimizationLevel(shaderc_optimization_level_performance);

		
		for (auto&& [type, source] : m_UncompiledShaderSources)
		{
			

			shaderc::SpvCompilationResult results = compiler.CompileGlslToSpv(source, Utils::FromVKShaderTypeToSPIRV(type), m_Name.c_str());

			if (results.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				LOG("%s\n", results.GetErrorMessage().c_str());
			}

			m_CompiledShaderSources[type] = std::vector<uint32_t>(results.cbegin(), results.cbegin());
		}

	}

	VkShaderModule Shader::CreateModule(const std::vector<uint32_t>& sprvCode)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = sprvCode.size();
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
			VkPipelineShaderStageCreateInfo stageInfo;
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = Utils::DeduceShaderStageFromType(type);
			stageInfo.pName = "main";
			stageInfo.module = module;

			shaderStages.push_back(stageInfo);
		}


		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; 

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

		vkCreatePipelineLayout(Application::Get().GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
		


	}

	void Shader::Reflect()
	{

	}

}
