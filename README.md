OUYA/Android port of Second Reality
===================================

This is a port of Second Reality by Future Crew focused to run on the OUYA platform (but should work fine on any other Android 2.3 device or higher).

**Current status**:

- Part 1 (alku): Renders and executes fully.  Bugs: Text does not scroll yet (needs ascrolltext() function implemented), and the spaceships/explosion effect is skipped.
- Part 2 (beg): Renders and executes fully.  Bugs: Slight graphic distortion before the title screen fades in.
- No audio playback code has been written yet.

![Running on a Kindle Fire HD](http://falken42.github.com/sr5.jpg)

*Commit c8da9a9 running on a Kindle Fire HD*

To build, clone the repository and run 'make' in the source folder.  The built .apk will automatically be uploaded to an attached device (via adb install) if it is connected.

The demo's runtime progress can be viewed with adb logcat.  The demo will automatically terminate after the first part completes.

If you are interested in helping, Pull Requests are welcomed.


Porting Notes
=============

This port basically implements a VGA emulation layer by hooking functions at the C library level.  This allows the demo to run natively while making as little code changes to the original source as possible.

For example, the macro MK\_FP() which is used to create a far pointer to a memory block has been replaced with a function.  This function detects if the memory block being requested is in the DOS VGA memory area (A000:0000), and if it is, it returns a pointer to an emulated VGA memory buffer.

Another function, outport(), is used to directly access the VGA hardware.  Since the C library no longer provides this function, we provide it instead, and emulate the VGA hardware accesses being made at runtime.

The demo calls an internal function called dis\_exit() to determine if the ESC key has been pressed and the demo should quit.  This function is called throughout the entire code, and is the main hook used for rendering.  After a simulated VBlank has passed (16.667ms @ 60fps), the current VGA buffers and palette are combined into a frame buffer, uploaded as a texture to OpenGL, and rendered as a fullscreen polygon.

Instead of handling each part as a separate executable, this port will simply compile and statically link all of the parts together and call them directly in the order of the original demo (alleviating a lot of x86 assembly code).


Older Screenshots
=================

![Running on a Kindle Fire HD](http://falken42.github.com/sr.jpg)

*Commit a762b8f running on a Kindle Fire HD, before Mode X support*

![Running on a Kindle Fire HD](http://falken42.github.com/sr2.jpg)

*Commit 2e7c444 running on a Kindle Fire HD, with basic Mode X support*

![Running on a Kindle Fire HD](http://falken42.github.com/sr3.jpg)

*Commit fec75a6 running on a Kindle Fire HD, with improved VGA support and scrolling*

![Running on a Kindle Fire HD](http://falken42.github.com/sr4.jpg)

*Commit 52a9fac running on a Kindle Fire HD, with various bug fixes*


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
