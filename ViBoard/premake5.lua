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
		"SDL3",
		"SDL3_image",
		"opengl32"
	}

	defines {
		"VI_EXECUTEABLE_NAME=\"%{prj.name}\""
	}

	filter "platforms:x64"
		architecture "x86_64"
	
	filter "platforms:x86"
		architecture "x86"

	filter { "action:vs*", "platforms:x64" }
		libdirs {
			"../dependencies/SDL3/vc/x64",
			"../dependencies/SDL3_image/vc/x64"
		}

	filter { "action:vs*", "platforms:x86" }
		libdirs {
			"../dependencies/SDL3/vc/x86",
			"../dependencies/SDL3_image/vc/x86"
		}

	filter { "toolset:mingw or gcc", "platforms:x64" }
		libdirs {
			"../dependencies/SDL3/mingw/x64/lib",
			"../dependencies/SDL3_image/mingw/x64/lib"
		}

	filter { "toolset:mingw or gcc", "platforms:x86" }
		libdirs {
			"../dependencies/SDL3/mingw/x86/lib",
			"../dependencies/SDL3_image/mingw/x86/lib"
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