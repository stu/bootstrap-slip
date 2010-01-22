solution "slip"
	configurations { "Debug", "Release" }

	project "slip"
		kind "ConsoleApp"
		language "C"
		files { "*.h", "*.c", "slip_tokeniser.c" }
		excludes { "slip_tokeniser.re2c" }

	configuration "windows"
		links { "kernel32" }
		prebuildcommands { "re2c.exe -o slip_tokeniser.c slip_tokeniser.re2c"}

	configuration "linux"
		links { "m" }
		prebuildcommands { "re2c -o slip_tokeniser.c slip_tokeniser.re2c"}

	configuration "Debug"
		defines { "DEBUG", "MEMWATCH"}
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize" }

