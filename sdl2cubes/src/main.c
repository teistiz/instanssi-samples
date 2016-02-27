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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct Transform { float m[16]; } Transform;

Transform tfIdentity();
Transform tfMultiply(Transform *a, Transform *b);

typedef struct TransformStack {
    unsigned size;
    unsigned pos;
    Transform t[];
} TransformStack;

void tfsClear(TransformStack *tfs) {
    tfs->t[0] = tfIdentity();
    tfs->pos  = 0;
}

void createTFStack(TransformStack **p, unsigned maxSize) {
    assert(maxSize > 0);
    size_t size = offsetof(TransformStack, t) + sizeof(Transform) * maxSize;
    *p = (TransformStack *)malloc(size);
    (*p)->size = maxSize;
    tfsClear(*p);
}

Transform tfsGet(TransformStack *tfs) { return tfs->t[tfs->pos]; }

void tfsPop(TransformStack *tfs) {
    assert(tfs->pos > 0);
    tfs->pos--;
}

void tfsPush(TransformStack *tfs, Transform tf) {
    assert(tfs->pos < tfs->size - 1);
    Transform *prev = &(tfs->t[tfs->pos]);
    tfs->pos++;
    tfs->t[tfs->pos] = tfMultiply(prev, &tf);
}

Transform tfIdentity() {
    Transform tf;
    float *m = tf.m;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i * 4 + j] = (i == j) ? 1 : 0;
        }
    }
    return tf;
}

Transform tfMultiply(Transform *a, Transform *b) {
    Transform tf;
    float *am = a->m, *bm = b->m;
    float *m = tf.m;
    for(int j = 0; j < 4; ++j) {
        for(int i = 0; i < 4; ++i) {
            m[i * 4 + j] = 0;
            for(int k = 0; k < 4; ++k) {
                m[i * 4 + j] += am[i * 4 + k] * bm[k * 4 + j];
            }
        }
    }
    return tf;
}

Transform tfTranslate(float x, float y, float z) {
    Transform tf = tfIdentity();

    tf.m[12] = x;
    tf.m[13] = y;
    tf.m[14] = z;
    return tf;
}

Transform tfScale(float s) {
    Transform tf;
    memset(&tf, 0, sizeof(tf));
    tf.m[0]  = s;
    tf.m[5]  = s;
    tf.m[10] = s;
    tf.m[15] = 1;
    return tf;
}

Transform tfScale3(float x, float y, float z) {
    Transform tf;
    memset(&tf, 0, sizeof(tf));
    tf.m[0]  = x;
    tf.m[5]  = y;
    tf.m[10] = z;
    tf.m[15] = 1;
    return tf;
}

Transform tfRotate(float angle, float x, float y, float z) {
    Transform t = tfIdentity();
    float *m    = t.m;

    float c = cosf(angle);
    float s = sinf(angle);
    float r = 1.0f - c;

    m[0]  = r * x * x + c;
    m[1]  = r * y * x + z * s;
    m[2]  = r * x * z - y * s;
    m[4]  = r * x * y - z * s;
    m[5]  = r * y * y + c;
    m[6]  = r * y * z + x * s;
    m[8]  = r * x * z + y * s;
    m[9]  = r * y * z - x * s;
    m[10] = r * z * z + c;
    return t;
}

Transform tfPerspective(float near, float far, float aspect, float fov_y) {
    Transform tf = tfIdentity();

    float *m = tf.m;
    float f  = cos(fov_y * 0.5f) / sin(fov_y * 0.5f);

    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (near + far) / (near - far);
    m[11] = -1.0f;
    m[14] = 2.0f * (near * far) / (near - far);
    m[15] = 0.0f;
    return tf;
}

Transform g_tfProjection;
TransformStack *g_tfsView;

typedef struct RenderMesh {
    GLuint vertexArray; // vertex array object id
    GLuint buffer;      // buffer object id
    unsigned vertices;  // number of vertices
} RenderMesh;

const char *WINDOW_TITLE = "ebin_cubes";
enum { AUDIO_SAMPLES     = 8192 };

SDL_Window *g_sdlWindow = NULL;
int g_windowWidth       = 1280;
int g_windowHeight      = 720;
int g_fullScreen        = 0;
int g_paused            = 0;
unsigned g_mouseButtons = 0;
float g_aspect          = 16.0f / 9.0f;

SDL_AudioDeviceID g_audioDevice;

Uint32 g_ticks, g_lastTicks;
float g_time = 0;

GLuint g_shaderBlobs = 0;
GLuint g_texFace     = 0;
GLuint g_texAtlas    = 0;

GLuint g_bufQuad = 0;
GLuint g_vaQuad  = 0;

GLint g_glUniformAlignment = 0;

RenderMesh g_meshSphere;

// This can be passed to shaders as-is, as long as it matches GLSL's layout
// rules. Basically things are aligned to their size, except that
// vec3 aligns like vec4 and matrices align to their column size
// (vec2/vec3/vec4). Arrays align to their elements' size (iirc).
typedef struct MeshUniforms {
    Transform projection;
    Transform view;
} MeshUniforms;

RenderMesh loadMeshToArray(const char *filename);
void addMeshSource(RenderMesh *dest, const char *filename) {
    // TODO: Implement like the shader one
    *dest = loadMeshToArray(filename);
}

GLuint g_ubGlobals = 0;

int initDemo();
int runDemo(float dt);

typedef struct AudioState { AudioReader *reader; } AudioState;

AudioState g_audioState;
void generateAudio(void *userdata, Uint8 *stream, int len);

void handleWindowResize(int width, int height) {
    g_windowWidth  = width;
    g_windowHeight = height;
    g_aspect = (float)width / height;
    glViewport(0, 0, width, height);
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
    g_sdlWindow = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, g_windowWidth,
                                   g_windowHeight, flags);

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

    SDL_DestroyWindow(g_sdlWindow);
    SDL_Quit();

    return 0;
}

void generateAudio(void *userdata, Uint8 *stream, int len) {
    AudioState *state = (AudioState *)userdata;
    int ofs           = arRead(state->reader, stream, len);
    // set any remaining buffer to 0
    if(len - ofs) {
        memset(stream + ofs, 0, len - ofs);
    }
}

void initBuffers();

// demo init code goes here.
int initDemo() {
    g_tfProjection = tfIdentity();
    createTFStack(&g_tfsView, 64);
    addShaderSource(&g_shaderBlobs, "res/test_vertex.glsl",
                    "res/test_fragment.glsl", NULL, NULL);

    addMeshSource(&g_meshSphere, "res/unitcube.obj");

    reloadShaders();
    initBuffers();
    g_texFace           = loadImageToTexture("res/quality_graphics.png");
    g_audioState.reader = arInit("res/test.ogg");
    glUseProgram(g_shaderBlobs);
    return 1;
}

void drawQuad() {
    glBindVertexArray(g_vaQuad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void initBuffers() {
    // Get new buffer handle for our uniforms (shader per-draw variables).
    glGenBuffers(1, &g_ubGlobals);
    glBindBuffer(GL_UNIFORM_BUFFER, g_ubGlobals);
    // GL_DYNAMIC_DRAW hints the buffer is for drawing with frequent updates
    // See: https://www.opengl.org/sdk/docs/man/html/glBufferData.xhtml .
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MeshUniforms), NULL,
                 GL_DYNAMIC_DRAW);

    float quadVertices[] = {
        1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
    };

    // make some data for a rectangle so we can render stuff
    glGenBuffers(1, &g_bufQuad);
    glBindBuffer(GL_ARRAY_BUFFER, g_bufQuad);
    // GL_STATIC_DRAW hints this is static (e.g. mesh) data for drawing
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    // Vertex Array Objects store vertex input state.
    // When we enable vertex arrays and assign pointers, the calls modify
    // the currently bound Vertex Array.
    // NOTE: The GL_ELEMENT_ARRAY_BUFFER binding is part of the VAO's state.
    // If you need to bind those somewhere else (e.g. to generate them),
    // make a scratchpad VAO just for that or you may experience pain.
    glGenVertexArrays(1, &g_vaQuad);
    glBindVertexArray(g_vaQuad);
    glEnableVertexAttribArray(0); // enable the VAO's attrib #0
    // Set vertex attribute #0 to read 2x float (vec2) from the currently
    // bound ARRAY_BUFFER, with no normalization, 0 stride (= tight packing)
    // starting from offset 0 in the buffer.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

/**
 * Demo script and rendering goes here.
 * Return 0 to quit.
 */
int runDemo(float dt) {
    if(!g_paused) {
        g_time += dt;
    }
    // if our demo had a script, this would be a good point
    // to check if stuff is habbening

    // render and present
    glClearColor(.2f, .2f, .2f, 0);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(g_shaderBlobs);

    // ---- pass per-frame globals ----
    tfsClear(g_tfsView); // reset transform stack
    float s = 1.0 / sqrtf(2);
    tfsPush(g_tfsView, tfRotate(g_time * 0.5f, 0, s, s));
    tfsPush(g_tfsView, tfScale(0.5f));

    MeshUniforms uf;
    uf.projection = tfScale3(1 / g_aspect, 1, 1);
    uf.view       = tfsGet(g_tfsView);

    // select our uniform buffer again
    glBindBuffer(GL_UNIFORM_BUFFER, g_ubGlobals);
    // upload new data
    // if this is still slow on AMD, make more buffers and cycle between them
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MeshUniforms), &uf);
    // set uniform binding #0 to point at the buffer
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, g_ubGlobals, 0,
                      sizeof(MeshUniforms));

    // bind the test texture to sampler 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texFace);

    // this restores the vertex array bindings we made earlier
    glBindVertexArray(g_meshSphere.vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, g_meshSphere.vertices);

    // show what we just drew
    SDL_GL_SwapWindow(g_sdlWindow);
    return 1;
}

RenderMesh loadMeshToArray(const char *filename) {
    Mesh *meshObj = meshReadOBJ(filename);
    RenderMesh renderMesh;

    memset(&renderMesh, 0, sizeof(RenderMesh));
    if(!meshObj) { // obj load failed
        return renderMesh;
    }
    renderMesh.vertices = meshGetNumVertices(meshObj);
    unsigned stride     = sizeof(float) * (3 + 2 + 3);

    glGenBuffers(1, &renderMesh.buffer);
    glBindBuffer(GL_ARRAY_BUFFER, renderMesh.buffer);
    glBufferData(GL_ARRAY_BUFFER, renderMesh.vertices * stride, NULL,
                 GL_STATIC_DRAW);

    void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    meshPackVertices(meshObj, (float *)ptr);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glGenVertexArrays(1, &renderMesh.vertexArray);
    glBindVertexArray(renderMesh.vertexArray);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    // Bind vertex inputs for vertex positions, texture coords and normals.
    // The offsets (last argument) are set relative to the ARRAY_BUFFER
    // bound when glVertexAttribPointer is called.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          (void *)(sizeof(float) * 3));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)(sizeof(float) * (3 + 2)));

    glBindVertexArray(0);
    return renderMesh;
}
