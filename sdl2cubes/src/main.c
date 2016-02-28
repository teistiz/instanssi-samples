/**
 * main.c
 * More cubes! This time in C11 with  SDL2 and OpenGL 3.3 Core.
 * License: WTFPL
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assert.h>

#include "image.h"
#include "audio.h"
#include "shaders.h"
#include "mesh_obj.h"

#define CUBES_DEBUG 0

extern int runDemo(float dt);
extern int initDemo();
extern void resizeDemo();
extern const char *WINDOW_TITLE;

SDL_Window *g_sdlWindow = NULL;

int g_windowWidth       = 1280;
int g_windowHeight      = 720;
int g_fullScreen        = 0;
int g_paused            = 0;
unsigned g_mouseButtons = 0;
float g_aspect          = 16.0f / 9.0f;

int g_glUniformAlignment = 0;
Uint32 g_ticks, g_lastTicks;

typedef struct AudioState { AudioReader *reader; } AudioState;
AudioState g_audioState = {NULL};

enum { AUDIO_SAMPLES = 8192 };
SDL_AudioDeviceID g_audioDevice;

void playAudio(const char *filename);
void generateAudio(void *userdata, Uint8 *stream, int len);
void presentWindow();
void handleWindowResize(int width, int height);

#if defined(__GNUC__)
#define UNUSED(x) x __attribute__((unused))
#else
#define UNUSED(x) x
#endif

void
#ifdef WINDOWS
    __attribute__((__stdcall__))
#endif
    debugCallback(GLenum UNUSED(source), GLenum UNUSED(type),
                  GLuint UNUSED(id), GLenum UNUSED(severity),
                  GLsizei UNUSED(length), const GLchar *message,
                  const GLvoid UNUSED(*data)) {
    printf("%s\n", message);
}

int main(int argc, char *argv[]) {
    SDL_Event event;
    int running = 1;

    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "--fullscreen") == 0) {
            g_fullScreen = 1;
        }
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // https://wiki.libsdl.org/SDL_CreateWindow
    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if(g_fullScreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    g_sdlWindow =
        SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         g_windowWidth, g_windowHeight, flags);

    if(!g_sdlWindow) {
        fprintf(stderr, "CreateWindow failed! Is the resolution supported?\n");
        return 1;
    }

    // Set up GL context for the window.
    // OpenGL 3.3 Core is available on just about everything,
    // including Windows, free Linux drivers and OS X.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    if(!SDL_GL_CreateContext(g_sdlWindow)) {
        fprintf(stderr, "GL_CreateContext failed!\n");
        return 1;
    }
    glewExperimental = 1; // needed for extension fetching in core contexts
    if(glewInit() != GLEW_OK) {
        fprintf(stderr, "glewInit failed!\n");
        return 1;
    }
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &g_glUniformAlignment);

    if(CUBES_DEBUG && GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(&debugCallback, NULL);
        // disable all messages
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                              NULL, GL_FALSE);
        // enable some critical messages
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                              GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,
                              GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,
                              GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
    }

    if(!initDemo()) {
        fprintf(stderr, "initDemo failed!\n");
        return 1;
    }
    if(g_audioState.reader) {
        // https://wiki.libsdl.org/CategoryAudio
        SDL_AudioSpec requested, got;
        requested.freq     = arGetRate(g_audioState.reader);
        requested.channels = arGetChannels(g_audioState.reader);
        requested.format   = AUDIO_S16;
        requested.samples  = AUDIO_SAMPLES;
        requested.callback = generateAudio;
        requested.userdata = &g_audioState;
        g_audioDevice      = SDL_OpenAudioDevice(NULL, 0, &requested, &got, 0);

        // start playing sound
        SDL_PauseAudioDevice(g_audioDevice, 0);
    }
    g_lastTicks           = SDL_GetTicks();
    unsigned frameCounter = 0;
    while(running) {
        // https://wiki.libsdl.org/CategoryEvents
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_WINDOWEVENT: {
                SDL_WindowEvent ev = event.window;
                if(ev.event == SDL_WINDOWEVENT_RESIZED) {
                    handleWindowResize(ev.data1, ev.data2);
                }
                break;
            }
            case SDL_KEYDOWN: {
                if(event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                } else if(event.key.keysym.sym == SDLK_F5) {
                    reloadShaders();
                } else if(event.key.keysym.sym == SDLK_p) {
                    g_paused = !g_paused;
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                // SDL_MouseMotionEvent ev = event.motion;
                // relative mouse input is ev.xrel and ev.yrel
                break;
            }
            case SDL_MOUSEWHEEL: {
                // SDL_MouseWheelEvent ev = event.wheel;
                // wheel motion is ev.y
                break;
            }
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN: {
                SDL_MouseButtonEvent ev = event.button;
                unsigned mask = 0;
                mask |= (ev.button == SDL_BUTTON_LEFT) ? 1 : 0;
                mask |= (ev.button == SDL_BUTTON_RIGHT) ? 2 : 0;
                if(ev.state == SDL_PRESSED) {
                    g_mouseButtons |= mask;
                } else {
                    g_mouseButtons &= ~mask;
                }
                // if either one of the interesting buttons is down, grab mouse
                SDL_SetRelativeMouseMode(g_mouseButtons & 3 ? SDL_TRUE
                                                            : SDL_FALSE);
                break;
            }
            case SDL_QUIT: {
                running = 0;
                break;
            }
            default:; // not interesting enough.
            }
        }

        g_ticks     = SDL_GetTicks();
        float dt    = (g_ticks - g_lastTicks) * 0.001f;
        g_lastTicks = g_ticks;

        // run demo, check if we're done yet
        running &= runDemo(dt);
        frameCounter++;
    }

    SDL_CloseAudioDevice(g_audioDevice);
    SDL_DestroyWindow(g_sdlWindow);
    SDL_Quit();

    return 0;
}

void setSoundtrack(const char *filename) {
    assert(!g_audioState.reader);
    g_audioState.reader = arInit(filename);
}

void setWindowTitle(const char *title) {
    SDL_SetWindowTitle(g_sdlWindow, title);
}

void generateAudio(void *userdata, Uint8 *stream, int len) {
    AudioState *state = (AudioState *)userdata;
    int ofs           = arRead(state->reader, stream, len);
    // set any remaining buffer to 0
    if(len - ofs) {
        memset(stream + ofs, 0, len - ofs);
    }
}

void presentWindow() { SDL_GL_SwapWindow(g_sdlWindow); }

void handleWindowResize(int width, int height) {
    g_windowWidth  = width;
    g_windowHeight = height;
    g_aspect = (float)width / height;
    glViewport(0, 0, width, height);
    resizeDemo();
}
