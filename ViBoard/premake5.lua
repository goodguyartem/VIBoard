project "ViBoard"
	language "C++"
	cppdialect "C++20"
	staticruntime "On"
	targetdir "bin/%{cfg.buildcfg}/%{cfg.architecture}"
	objdir "bin/intermediates/%{cfg.buildcfg}/%{cfg.architecture}"

	files { 
		"src/**.h", 
		"src/**.cpp",
		"res/**"
	}

	includedirs {
		"../dependencies/imgui",
		"../dependencies/imgui/backends",
		"../dependencies/SDL3/include",
		"../dependencies/SDL3_image/include",
		"../dependencies/json/single_include",
		"../dependencies/minimp3"
	}

	links {
		"ImGui",
		"SDL3.lib",
		"SDL3_image.lib",
		"opengl32.lib"
	}

	defines {
		"VI_EXECUTEABLE_NAME=\"%{prj.name}\""
	}

	filter "platforms:x64"
		architecture "x86_64"
		libdirs {
			"../dependencies/SDL3/lib/x64",
			"../dependencies/SDL3_image/lib/x64"
		}
	
	filter "platforms:x86"
		architecture "x86"
		libdirs {
			"../dependencies/SDL3/lib/x86",
			"../dependencies/SDL3_image/lib/x86"
		}

	filter "system:windows"
		systemversion "latest"
		defines { "VI_PLATFORM_WINDOWS" }

	filter "configurations:Debug"
		kind "ConsoleApp"
		defines { "_Debug", "VI_LOG_LEVEL=6" }
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		kind "WindowedApp"
		entrypoint "mainCRTStartup"
		defines { "VI_LOG_LEVEL=0" }
		runtime "Release"
		optimize "On"
		symbols "Off"

	filter "action:vs*"
		defines { "VI_MSVC" }