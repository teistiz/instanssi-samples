# This NMake makefile can build main.c using only Microsoft tools
# (and Crinkler). Behold its form and despair.

# msvcrt.lib
LIBS = user32.lib kernel32.lib opengl32.lib glu32.lib gdi32.lib dsound.lib
CFLAGS = /GS- /Os /fp:fast /Gr /EHsc /Gs16777216 /QIfist /Oi /W4

CRNKFLAGS = /SUBSYSTEM:WINDOWS /TRANSFORM:CALLS /HASHSIZE:200 /RANGE:opengl32

# Build with no debugging functionality but weak compression.
# Useful for quick testing.
normal : main.normal.obj
    crinkler main.normal.obj /OUT:main.exe $(CRNKFLAGS) $(LIBS)

# Build with additional debug luxuries like logging. This requires linking in
# a compatible MSVC runtime (like the one distributed with Visual C++ 6.0).
debug : main.debug.obj
    crinkler main.debug.obj /OUT:main.exe $(CRNKFLAGS) $(LIBS) msvcrt.lib

# Build the release version. This can take some time.
release : main.release.obj
    crinkler main.release.obj /OUT:main.exe $(CRNKFLAGS) /COMPMODE:SLOW /ORDERTRIES:5000 /RANGE:opengl32 $(LIBS)

clean :
    -del main.debug.obj >NUL 2>&1
    -del main.release.obj >NUL 2>&1
    -del main.normal.obj >NUL 2>&1

main.debug.obj : main.c
    cl /c $(CFLAGS) /DDEBUG /Fomain.debug.obj main.c

main.release.obj : main.c
    cl /c $(CFLAGS) /Fomain.release.obj main.c

main.normal.obj : main.c
    cl /c $(CFLAGS) /Fomain.normal.obj main.c
