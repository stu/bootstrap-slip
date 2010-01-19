solution "slip"
	configurations { "Debug", "Release" }

	project "slip"
		kind "ConsoleApp"
		language "C"
		files { "*.h", "*.c" }
		excludes { "tools/*", "split_tokeniser.re2c" }
		prebuildcommands { "re2c -o  slip_tokeniser.c slip_tokeniser.re2c"}

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
