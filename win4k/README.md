# README

This is a stub 4k demo made by stripping the Instanssi 2011 winner "fail4k" of its messy effect code and cleaning up what remained. It is meant as an example of how to set up OpenGL and DirectSound on Windows without depending on C runtime libraries.

## Building

To build this, you need at least:

* The Crinkler executable file compressor
* A compatible Microsoft C/C++ compiler
* Windows platform libraries

The "Microsoft Visual C++ Compiler for Python 2.7" has a slightly older version of the C/C++ compiler that works well, and comes with the required libraries.

The included NMake compatible makefile can build the project either in normal, debug or release mode. The normal mode includes minimal extras and may work well for dev

The debug build enables some logging functions and checks that require a C runtime library to work; unfortunately, Crinkler is not compatible with many versions of msvcrt.lib.
