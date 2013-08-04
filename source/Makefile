default: bin/secondreality-debug.apk

bin/secondreality-debug.apk: local.properties jni/main.c
	@ndk-build NDK_DEBUG=0
	@ant debug
	@mv bin/secondreality-debug.apk bin/secondreality.apk
	@-adb uninstall jp.falken42.secondreality
	@adb install bin/secondreality.apk

local.properties:
	@android update project -p . -n secondreality

clean:
	rm -rf bin obj libs

fullclean: clean
	rm -f build.xml local.properties proguard-project.txt

