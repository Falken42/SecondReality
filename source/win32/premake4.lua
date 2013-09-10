-- To build with VS2010:
-- 1. premake4 vs2010
-- 2. Open the generated solution

-- To build with clang:
-- 1. premake4 gmake
-- 2. mingw32-make CC=clang

solution "u2"
	configurations {"debug", "release"}
	project "u2"
		kind "WindowedApp"
		flags {"WinMain"}
		language "C"
		files {
			"../jni/u2-port.c",
			"../jni/platform-win32.c",
			"../jni/sin1024.c",
			"../jni/u2/alku/main.c",
			"../jni/u2/pam/outtaa.c",
			"../jni/u2/pam/pam-asm.c",
			"../jni/u2/beg/beg.c",
			"../jni/u2/glenz/glenz-main.c",
			"../jni/u2/glenz/glenz-asm.c",
			"../jni/u2/glenz/zoomer.c",
			"../jni/u2/dots/dots-main.c",
			"../jni/u2/dots/dots-asm.c"
		}
		targetdir "."
		if _ACTION == "vs2010" then
			buildoptions {
				"/wd4013", -- 'function' undefined; assuming extern returning int
				"/wd4101", -- 'identifier' : unreferenced local variable
				"/wd4554", -- 'operator' : check operator precedence for possible error; use parentheses to clarify precedence
				"/J" -- default char type is unsigned
			}
		elseif _ACTION == "gmake" then -- clang
			buildoptions {
				"-fno-color-diagnostics",
				"-funsigned-char",
				"-Wno-unused-value"
			}
		end
		configuration "debug"
			targetsuffix "d"
			defines {"DEBUG"}
			flags {"Symbols"}
		configuration "release"
			defines {"NDEBUG"}
			flags {"Optimize"}
