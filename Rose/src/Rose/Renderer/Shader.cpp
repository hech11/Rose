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
		//VkResult validation = vkCreateShaderModule(, &createInfo, nullptr, &result); //TODO: Set up device!

		return result;
	}

	void Shader::CreateShaderStagePipeline()
	{
		for (auto&& [type, module] : m_ShaderModules)
		{
			VkPipelineShaderStageCreateInfo stageInfo;
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = Utils::DeduceShaderStageFromType(type);
			stageInfo.pName = "main";
			stageInfo.module = module;
		}
	}

	void Shader::Reflect()
	{

	}

}
