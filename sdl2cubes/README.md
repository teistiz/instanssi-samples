# sdl2cubes

Basic multiplatform framework for intros and other realtime graphics stuff using SDL2, OpenGL 3.3+ and libvorbis.

If pointer arithmetic, memory alignment rules and other things confuse you, you may either learn something or this or become even more confused.

## Debugging

Use apitrace! It can record the GL calls your program makes, view them and play them back later with error checking. You can also inspect the GL state at each call to see if your shaders are getting data or garbage.

## Building

The source code has been known to build successfully on Debian and Windows (with MSYS2).

### Linux

The included Makefile should work as-is on most Linux distributions. You'll need a compiler toolchain (e.g. the "build-essentials" package on Ubuntu-likes) and development files for SDL2 and ogg, vorbis and vorbisfile.

### Windows (Visual C++ 2015)

TODO: ^

### Windows (MSYS2)

The included Visual Studio Code project is set up to build this using GNU Make (Ctrl+Shift+B), but you'll need a MinGW-like toolchain. Installing MSYS2 is probably the easiest way to acquire one.

After installing, open MSYS2's shell (`msys2_shell.bat`), sync the package database and update the MSYS2 core with:

    pacman -Sy
    pacman --needed -S bash pacman pacman-mirrors msys2-runtime

If the second command updates anything, restart the shell. If you want to, you can update the rest of the packages with:

    pacman -Su

Then install the project dependencies:

    pacman -S make mingw-w64-i686-SDL2 mingw-w64-i686-libvorbis mingw-w64-i686-gcc

Many common libraries are available through the MSYS2 package repositories. You can search for additional packages using:

    pacman -Ss partialname

Most libraries are available for both 64- and 32-bit targets. The instructions here assume a 32-bit target. You can also place your third party dependencies' libraries and headers in the `deps/` folder. Again, mind the target architecture.

Note that the `msys_make.bat` script used by the Visual Studio Code project to invoke Make assumes MSYS2 is installed to `C:\msys32`; if this is not the case, just edit the batch file.
