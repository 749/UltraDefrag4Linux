----------------------- Lua 5.1 for Ultra Defragmenter ------------------------

RELEASE NOTES:

Here are sources of a special version of Lua 5.1. With os.shellexec() call
and a few other little improvements for Win32 environment.

The *.build files are included to produce Lua modules for three processor
architectures (i386, amd64 and ia64) during the automatic UltraDefrag
build process.

Note: Lua produced by Windows DDK isn't compatible with standard Lua package,
because there are used __stdcall calling conventions instead of __cdecl.
This is a well known DDK bug - it is not compatible with ANSI C Standard.
Due to this reason, all the compiled binaries have the 'a' suffix in their names.
We believe that it will prevent overwrite of standard Lua binaries if they are
present on a target system.

Lua 5.1 is licensed under the terms of the MIT license reproduced below.

Copyright © 1994-2007 Lua.org, PUC-Rio.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
