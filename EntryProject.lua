project "WorkEncourager"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++20"
	characterset "Unicode"

	files
	{
		"src/**.cpp",
		"include/**.h",
		"res/**.rc",
		"res/**.ico",
		"res/**.manifest"
	}

	includedirs
	{
		"include"
	}

	links
	{
		"d2d1",
		"dwrite",
		"dwmapi",
		"user32",
		"gdi32",
		"shell32",
		"ole32"
	}

	defines
	{
		"UNICODE",
		"_UNICODE",
		"WIN32_LEAN_AND_MEAN",
		"NOMINMAX",
		"_WIN32_WINNT=0x0A00",
		"WINVER=0x0A00"
	}

	buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus", "/utf-8", "/permissive-", "/MP" }

	warnings "Extra"

	location (LocationDir)
	targetdir (EXEDir)
	objdir (ObjectDir)

	filter "system:windows"
		systemversion "latest"
		defines { "_WINDOWS" }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"
		optimize "Off"
		defines { "_DEBUG" }

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
		symbols "On"
		defines { "_RELEASE", "NDEBUG" }

	filter "configurations:Dist"
		runtime "Release"
		staticruntime "On"
		optimize "Speed"
		symbols "Off"
		defines { "_DIST", "NDEBUG" }
