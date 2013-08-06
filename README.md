OUYA/Android port of Second Reality
===================================

This is a port of Second Reality by Future Crew focused to run on the OUYA platform (but should work fine on any other Android 2.3 device or higher).

_Current status_: The first part of the demo executes, but only a black screen is visible.  No audio is available yet.

- Mode 13h VGA emulation and palette register sets is complete.
- First part (alku) compiles and fully executes to completion.
- The build system and the GL rendering backend is complete.

To build, clone the repository and run 'make' in the source folder.  The built .apk will automatically be uploaded to an attached device (via adb install) if it is connected.

The demo's runtime progress can be viewed with adb logcat.  The demo will automatically terminate after the first part completes.

If you are interested in helping, Pull Requests are welcomed.


Porting Notes
=============

The intent is to port this demo making as little code changes to the original source as possible.

The original code was written for DOS, and so is designed to execute a single function over a set of frames.  In other words, it is non-reentrant, which is different than the GL method of rendering where typically a render() function is called for each new frame.

Fortunately, the demo was built around a system (called the Demo Interrupt Server, or DIS) which processed the music, updated the frame counter, set new palettes to the hardware, and so on.  The DIS also checked if the ESC key was pressed to terminate the demo, and this flag is checked by calling the dis\_exit() function which is used everywhere within the code.  This is what will be our hook to perform rendering.

Since the original demo used VGA modes for display, an 8-bit indexed color buffer and palette for 256 colors will be provided by the demo.  This will need to be converted to a 24-bit RGB texture in order to be displayed by GL.

The demo does direct writes to memory location A000:0000 (the VGA frame buffer), which will need to be modified to point to our internal color buffer.  The outport() calls can be implemented to get updated palette information (and possibly other VGA mode info -- I haven't looked through the entire U2 codebase yet).

Since each part was designed as a separate .exe file, this port will simply compile and statically link all of the parts together directly and call them in the order of the original demo (alleviating a lot of x86 assembly code).


License
=======

The license for this port is the Unlicense, and is the same as the original Second Reality source ( https://github.com/mtuomi/SecondReality ):

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
