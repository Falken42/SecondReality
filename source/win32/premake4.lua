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
			"../jni/u2/dots/dots-main.c",
			"../jni/u2/dots/dots-asm.c"
		}
		targetdir "../assets"
		buildoptions {
			-- "-fno-color-diagnostics",
			"-funsigned-char",
			"-Wno-unused-value"
		}
		configuration "debug"
			targetsuffix "d"
			defines {"DEBUG"}
			flags {"Symbols"}
		configuration "release"
			defines {"NDEBUG"}
			flags {"Optimize"}
