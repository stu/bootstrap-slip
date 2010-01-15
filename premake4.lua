solution "slip"
	configurations { "Debug", "Release" }

   -- A project defines one build target
	project "slip"
		kind "ConsoleApp"
		language "C"
		files { "**.h", "**.c" }

	configuration "windows"
		links { "kernel32" }

	configuration "linux"
		links { "m" }

	configuration "Debug"
		defines { "DEBUG", "MEMWATCH"}
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize" }

