solution "slip"
	configurations { "Debug", "Release" }

	project "lemon"
		kind "ConsoleApp"
		language "C"
		files { "lemon/lemon.c" }
		targetdir ("lemon")

	configuration "Debug"
		defines { "DEBUG", "MEMWATCH"}
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize" }


	project "slip"
		kind "ConsoleApp"
		language "C"
		files { "*.h", "*.c" }
		excludes { "tools/*", "split_tokeniser.re2c", "split_parser.y" }
		prebuildcommands { "re2c -o  slip_tokeniser.c slip_tokeniser.re2c"}
		--prebuildcommands { "lemon/lemon -q slip_parser.y" }

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
