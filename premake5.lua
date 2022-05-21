workspace "Rose"
	architecture "x86_64"
	startproject "SandboxApplication"



	configurations
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}


outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["glfw"] = "Rose/vendor/glfw"
IncludeDir["glm"] = "Rose/vendor/glm"
IncludeDir["vulkan"] = "Rose/vendor/vulkan1.2.182.0"

group "Dependencies/Rendering"
	include "Rose/vendor/glfw"
group ""

group "Core"

	project "Rose"
		location "Rose"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"


		targetdir ("bin/" .. outputDir .. "/%{prj.name}");
		objdir ("bin-int/" .. outputDir .. "/%{prj.name}");

		--pchheader "RosePCH.h"
		--pchsource "Rose/src/RosePCH.cpp"


		files 
		{
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp",
			"%{prj.name}/src/**.hpp",
			"%{prj.name}/vendor/glm/glm/**.hpp",
			"%{prj.name}/vendor/glm/glm/**.inl",
			"%{prj.name}/vendor/imgui/*.h",
			"%{prj.name}/vendor/imgui/*.cpp",
			"%{prj.name}/vendor/imgui/backends/imgui_impl_vulkan.h",
			"%{prj.name}/vendor/imgui/backends/imgui_impl_vulkan.cpp",
			"%{prj.name}/vendor/imgui_internal.h",
			"%{prj.name}/vendor/imgui/imgui.h",
			"%{prj.name}/vendor/imgui/imgui.cpp",
			"%{prj.name}/vendor/imgui/imgui_demo.cpp",
			"%{prj.name}/vendor/imgui/imgui_draw.cpp",
			"%{prj.name}/vendor/imgui/imgui_tables.cpp",
			"%{prj.name}/vendor/imgui/imgui_widgets.cpp",
			--"%{prj.name}/vendor/stb_image/*.cpp",
			--"%{prj.name}/vendor/stb_image/*.h",
			"%{prj.name}/vendor/vulkan1.2.182.0/**.hpp",
			"%{prj.name}/vendor/vulkan1.2.182.0/**.h"
		}

		includedirs
		{
			"%{prj.name}/src",
			"%{prj.name}/vendor",
			"%{prj.name}/vendor/spdlog/include",
			"%{IncludeDir.glfw}/include",
			"%{IncludeDir.imgui}",
			"%{IncludeDir.glad}/include",
			"%{IncludeDir.glm}",
			"%{IncludeDir.VulkanSDK}/include/"

		}

		links
		{
			"glfw",
		}
		defines 
		{
			"_CRT_SECURE_NO_WARNINGS"
		}

		filter "system:windows"
			systemversion "latest"

			defines
			{
				"RS_PLATFORM_WINDOWS",
				"GLFW_INCLUDE_NONE",
				"NOMINMAX"
			}

		filter "configurations:Debug"
			defines {
				"RS_DEBUG"
			}

			runtime "Debug"
			symbols "on"


			libdirs
			{
				"%{prj.name}/vendor/vulkan1.2.182.0/debugLib"
			}


			links
			{

				"shaderc_sharedd.lib",
				"spirv-cross-cored.lib",
				"spirv-cross-glsld.lib",
				"vulkan-1.lib"
			}


		filter "configurations:Release"
			defines {
				"RS_RELEASE"
			}

			runtime "Release"
			optimize "on"


			libdirs
			{
				"%{prj.name}/vendor/vulkan1.2.182.0/lib"
			}


			links
			{

				"shaderc_shared.lib",
				"spirv-cross-core.lib",
				"spirv-cross-glsl.lib",
				"vulkan-1.lib"
			}

group ""


group "Tests"
project "SandboxApplication"
		location "SandboxApplication"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"


		targetdir ("bin/" .. outputDir .. "/%{prj.name}");
		objdir ("bin-int/" .. outputDir .. "/%{prj.name}");

		files 
		{
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp",
			"%{prj.name}/src/**.hpp"
		}


		links
		{
			"Rose",
		}

		defines 
		{
			"_CRT_SECURE_NO_WARNINGS"
		}


		includedirs
		{
			"Rose/src",
			"Rose/vendor/spdlog/include",
			"Rose/vendor",
			"%{IncludeDir.glfw}/include",
			"%{IncludeDir.imgui}",
			"%{IncludeDir.glm}",
			"%{prj.name}/src"
		}

		filter "system:windows"
			systemversion "latest"

			defines
			{
				"RS_PLATFORM_WINDOWS",
				"GLFW_INCLUDE_NONE",
			}

		filter "configurations:Debug"
			defines {
				"RS_DEBUG"
			}

			runtime "Debug"
			symbols "on"
			postbuildcommands
			{
				'{COPY} "../Rose/vendor/vulkan1.2.182.0/bin/debug/shaderc_sharedd.dll" "%{cfg.targetdir}"',
				'{COPY} "../Rose/vendor/vulkan1.2.182.0/bin/debug/spirv-cross-c-sharedd.dll" "%{cfg.targetdir}"',
				'{COPY} "../Rose/vendor/vulkan1.2.182.0/bin/debug/SPIRV-Tools-sharedd.dll" "%{cfg.targetdir}"'
			}

		filter "configurations:Release"
			defines {
				"RS_RELEASE"
			}

			postbuildcommands
			{
				'{COPY} "../Rose/vendor/vulkan1.2.182.0/bin/release/shaderc_shared.dll" "%{cfg.targetdir}"',
				'{COPY} "../Rose/vendor/vulkan1.2.182.0/bin/release/spirv-cross-c-shared.dll" "%{cfg.targetdir}"',
				'{COPY} "../Rose/vendor/vulkan1.2.182.0/bin/release/SPIRV-Tools-shared.dll" "%{cfg.targetdir}"'
			}


			runtime "Release"
			optimize "on"

group ""