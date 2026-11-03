include "Directory.lua"
include "Dependencies.lua"

workspace "WorkEncourager"
	architecture "x64"
	startproject "EntryProject"
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	filter "system:windows"
		buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus", "/utf-8" }

group ""
	include "EntryProject.lua"
