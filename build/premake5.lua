local RootDirectory = "../"

workspace "MandelbrotSet"
	location(RootDirectory)
	entrypoint "wWinMainCRTStartup"  

	configurations 
	{
		"Debug",
		"Release",
	}

	platforms 
	{
		"x64", 
	}

	filter "configurations:Debug"
		defines "APP_DEBUG"

project "MandelbrotSet"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir (RootDirectory .. "bin_%{cfg.buildcfg}_%{cfg.platform}") -- where the output binary goes.
    targetname "MandelbrotSet" -- the name of the executable saved to targetdir

	local ProjectSourceDirectory = RootDirectory .. "MandelbrotSet/"
	files
	{
		ProjectSourceDirectory .. "**.h",
		ProjectSourceDirectory .. "**.hpp",
		ProjectSourceDirectory .. "**.c",
		ProjectSourceDirectory .. "**.cpp",
	}

	includedirs
	{
		ProjectSourceDirectory,
		ProjectSourceDirectory .. "vendor/glm",
		ProjectSourceDirectory .. "vendor/glm/glm",
	}

	libdirs
    {
      -- add dependency directories here
    }

    links
    {
		RootDirectory .. "MandelbrotSet/vendor/vulkan/lib/vulkan-1.lib"
    }