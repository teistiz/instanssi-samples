# README

This is a stub 4k demo made by stripping the Instanssi 2011 4k winner "fail4k" of its messy GPGPU particle code and sequencer, then cleaning up what remained. It is provided as an example of how to set up OpenGL and DirectSound on Windows without depending on C runtime libraries.

## Building

To build this, you need at least:

* The Crinkler executable file compressor
* A compatible Microsoft C/C++ compiler
* Some Windows platform libraries

The "Microsoft Visual C++ Compiler for Python 2.7" has a slightly older version of the C/C++ compiler that works well, and comes with the required libraries.

The included NMake compatible Makefile can build the project either in normal, debug or release mode. Just open the Visual C++ environment's prompt and say `nmake` in the project's directory to build.

The Makefile includes three build targets:

* The default target works well for fast development.
* The debug target enables some logging functions and checks that require a C runtime library to work. Only some versions of `msvcrt.lib` will work with Crinkler; see its manual for details.
* The release target features much better (and slower) compression. You may want to experiment with the compression settings.
