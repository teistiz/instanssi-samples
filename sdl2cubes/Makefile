.PHONY: clean debug release
.DEFAULT_TARGET: debug

# This requires GNU Make 3.82+, but allows the file to be handled with a tool
# that autoconverts tab characters to spaces.
.RECIPEPREFIX = >

OUT := sdl2cubes
SOURCES := $(wildcard src/*.c)    # build all C files
OBJECTS := $(SOURCES:.c=.o)

LIB_PATHS :=
INCLUDE_PATHS :=

ifdef SYSTEMROOT
    # probably Windows/MinGW
    OUT := $(OUT).exe
    CC := $(CC).exe

    CFLAGS += -static -DWINDOWS
    LDFLAGS += -static

    LIBS += mingw32 SDL2main SDL2.dll glew32 opengl32
    INCLUDE_PATHS += deps/include
    LIB_PATHS += deps/lib
else
    # probably Unix-ish
    LIBS += GLEW GL SDL2 m
    INCLUDE_PATHS += deps/include
    LIB_PATHS += deps/lib
endif

LIBS += vorbisfile vorbis ogg

# C is standard C11
#CFLAGS += -x c++ -std=gnu++11
CFLAGS += -std=c11 -pedantic-errors
# warn about many silly errors
CFLAGS += -Wall -Wextra -Wshadow
# This is where the system libs are.
CFLAGS += $(addprefix -isystem , $(INCLUDE_PATHS)) -I./include/
# This makes GCC and Clang automatically generate Makefile snippets that
# let us track header dependencies.
CFLAGS += -MMD
# Add our own dependency paths
LDFLAGS += $(addprefix -L, $(LIB_PATHS))
# Add the library files
LIBFLAGS += $(addprefix -l, $(LIBS))

# optimize release builds
release: CFLAGS += -O3
# debug info goes into debug builds
debug: CFLAGS += -g

debug: $(OBJECTS)
> $(CC) -g $(LDFLAGS) $(OBJECTS) -o $(OUT) $(LIBFLAGS)

release: $(OBJECTS)
> $(CC) $(LDFLAGS) $(OBJECTS) -o $(OUT) $(LIBFLAGS)
> strip $(OUT)

clean:
> rm -f $(OUT) $(OBJECTS) $(OBJECTS:%.o=%.d)

# Include auto-generated header dependencies.
-include $(OBJECTS:%.o=%.d)

%.o : %.c
> $(CC) -c $(CFLAGS) $< -o $@

TEST_SOURCES := $(wildcard tests/*.c)
TEST_OBJECTS := $(TEST_SOURCES:.c=.o)

test_mesh: CC := afl-gcc
test_mesh: src/mesh_obj.o tests/test_mesh.o
> afl-gcc tests/test_mesh.o src/mesh_obj.o -o test_mesh

