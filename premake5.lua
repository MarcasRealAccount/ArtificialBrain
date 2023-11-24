workspace("ArtificialBrain")
	common:addConfigs()
	common:addBuildDefines()

	cppdialect("C++latest")
	rtti("Off")
	exceptionhandling("On")
	flags("MultiProcessorCompile")

	startproject("ArtificialBrain")
	project("glad")
		location("glad/")
		warnings("Off")
		
		kind("StaticLib")
		common:outDirs(true)
		
		includedirs({ "%{prj.location}/include/" })
		files({
			"%{prj.location}/include/**",
			"%{prj.location}/src/**"
		})
		removefiles({ "*.DS_Store" })
	
	project("ArtificialBrain")
		location("ArtificialBrain/")
		warnings("Extra")

		common:outDirs()
		common:debugDir()

		filter("configurations:Debug")
			kind("ConsoleApp")
		filter("configurations:not Debug")
			kind("WindowedApp")
		filter({})

		includedirs({ "%{prj.location}/Src/" })
		files({ "%{prj.location}/Src/**" })
		removefiles({ "*.DS_Store" })

		links({ "glad" })
		externalincludedirs({ "%{wks.location}/glad/include/" })

		pkgdeps({ "commonbuild", "backtrace", "glfw" })

		common:addActions()