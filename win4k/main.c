/**
 * main.c
 * Some framework for 4k intros.
 */

#include <windows.h>
#include <wingdi.h>
#include <dsound.h>
#include <limits.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

typedef void (*funcp)(void);

HWND g_hWnd;
int g_windowWidth, g_windowHeight;
int g_running = 1;
GLuint g_shaderTest;

DWORD WINAPI audioThread(void *param);

// Some MS compilers may try to generate calls to helper functions like _ftol
// when they see a float-to-int conversion. This guarantees consistent rounding
// and error behavior, but may seriously bloat the code.
// Calling this is probably unnecessary if /QIfist works.
__forceinline int f32_to_i32(float f) {
    int i;
    __asm {
      fld f // load float f into x87 stack
      fistp i // convert into integer i and pop stack
    }
    return i;
}
__forceinline short f32_to_i16(float f) {
    short i;
    __asm {
      fld f
      fistp i // there are 16, 32 and 64 bit versions of this
    }
    return i;
}
__forceinline float sin_x87(float f) {
    __asm {
        fld f // load float f to stack
        fsin // get sine of stack top
        fstp f // pop the stack top to f
    }
    return f;
}

// ---- debug stuff ----

// Enabling this adds error checks and logging for shader compilation.
#ifdef DEBUG
#include <stdio.h>
FILE *logfile;
#define DEBUG_INIT(filename)                                                  \
    { logfile = fopen(filename, "wt"); }
#define LOG(param, ...)                                                       \
    {                                                                         \
        fprintf(logfile, param, ##__VA_ARGS__);                               \
        fflush(logfile);                                                      \
    }
#define DEBUG_CLOSE                                                           \
    { fclose(logfile); }
#define DEBUG_ASSERT(cond)                                                    \
    {                                                                         \
        if(!(cond)) {                                                         \
            LOG("assert fail: " #cond "\n");                                  \
            exit(1);                                                          \
        }                                                                     \
    }
#else // DEBUG
#define DEBUG_INIT(filename)                                                  \
    {}
#define LOG(param, ...)                                                       \
    {}
#define DEBUG_ASSERT(cond)                                                    \
    {}
#define DEBUG_CLOSE
#endif // DEBUG

#define SND_RATE 44100
#define SND_GAIN 5000.0f
#define SND_BUFFER_SAMPLES 16384
#define PI2 (3.14159f * 2.0f)

// ---- shader utilities ----

// let's avoid argument passing if possible, stacks are bloat >:)
const char *g_srcVert, *g_srcFrag, *g_srcGeom;
const char *g_srcComp;
GLenum g_compType;         // current shader component type
unsigned g_currentProgram; // program being compiled

unsigned buildGLProgram();

const static char *glnames[] = {
    "glCreateShader",       "glCreateProgram",
    "glShaderSource",       "glCompileShader",
    "glAttachShader",       "glLinkProgram",
    "glUseProgram",         "glGenFramebuffersEXT",
    "glBindFramebufferEXT", "glFramebufferTexture2DEXT",
    "glActiveTexture",      "glGetUniformLocation",
    "glUniform1i"};

static funcp glfunc[sizeof(glnames) / sizeof(char *)];

#define glCreateShader ((PFNGLCREATESHADERPROC)(glfunc[0]))
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)(glfunc[1]))
#define glShaderSource ((PFNGLSHADERSOURCEPROC)(glfunc[2]))
#define glCompileShader ((PFNGLCOMPILESHADERPROC)(glfunc[3]))
#define glAttachShader ((PFNGLATTACHSHADERPROC)(glfunc[4]))
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)(glfunc[5]))
#define glUseProgram ((PFNGLUSEPROGRAMPROC)(glfunc[6]))

#define glGenFramebuffers ((PFNGLGENFRAMEBUFFERSEXTPROC)(glfunc[7]))
#define glBindFramebuffer ((PFNGLBINDFRAMEBUFFEREXTPROC)(glfunc[8]))
#define glFramebufferTexture2D ((PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(glfunc[9]))

#define glActiveTexture ((PFNGLACTIVETEXTUREPROC)(glfunc[10]))
#define glGetUniformLocation ((PFNGLGETUNIFORMLOCATIONPROC)(glfunc[11]))
#define glUniform1i ((PFNGLUNIFORM1IPROC)(glfunc[12])) // needed for samplers

float quad_buf[] = {-1, +1, -1, -1, +1, -1, +1, +1};

// utility code for drawing quads
_inline void drawQuad() {
    glVertexPointer(2, GL_FLOAT, 0, quad_buf);
    glDrawArrays(GL_QUADS, 0, 4);
}

unsigned g_frameNumber = 0;

// execute demo for one frame
void runDemo() {
    g_frameNumber++;
    glEnableClientState(GL_VERTEX_ARRAY);
    // Pass something as a random seed.
    // These old GL built-ins can be useful for passing small pieces of data.
    glColor3f(g_frameNumber / 0.234f, g_frameNumber / 60.0f, 0);
    glClearColor(1, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(g_shaderTest);
    drawQuad();
}

// TODO: write a minifier for these?

const static char *GLSL_HEADER =
    "#version 130\n"
    "float rand(vec2 co, vec2 sd){"
    "return fract(sin(dot(co.xy - sd, vec2(12.9898, 78.233))) * 43758.5453);"
    "}";

// pass normalized vertex pos and two floats of other info to frag shader
const static char *VERTEX_PASSTHROUGH =
    "out vec4 sd;void main(){"
    "gl_Position = gl_Vertex;sd=vec4(gl_Vertex.xy,gl_Color.xy);}";

_inline void buildShaders() {
    g_srcVert = VERTEX_PASSTHROUGH;
    g_srcGeom = NULL;
    g_srcFrag =
        "in vec4 sd;void main(){vec2 nc=sd.xy;"     // grab normalized coords
        "vec2 ofs=vec2(sin(sd.w),cos(sd.w));"       // pattern centers
        "gl_FragData[0] = vec4((rand(sd.xy, sd.zw)" // random noise
        "+ sin(length(nc + ofs)*12.1+sd.w)"         // center 1
        "+ cos(length(nc + ofs.yx*0.5)*14.5+sd.w))*0.5+0.5); }"; // center 2
    g_shaderTest = buildGLProgram();
}

void initDemo() {
    buildShaders();
    // if the synthesizer needs init, could do it here or in the audio thread
}

// ---- audio synthesis ----

unsigned int g_audioSample = 0;
int *g_audioBuffer;

#define TEST_FREQ 60.0f

float wave(float t) {
    return sin_x87(PI2 * t * TEST_FREQ);
}

typedef struct { float a, d, s, h, r; } envelope_t;

float envelope(float t, const envelope_t *env) {
    float tmp = t;
    if(tmp < env->a) {
        return tmp / env->a;
    }
    tmp -= env->a;
    if(tmp < env->d) {
        return 1.0f - tmp / env->d * (1.0f - env->s);
    }
    tmp -= env->d;
    if(tmp < env->h) {
        return env->s;
    }
    tmp -= env->h;
    if(tmp < env->r) {
        return env->s - (tmp / env->r * env->s);
    }
    return 0;
}

envelope_t test_env = {0.001f, 0.01f, 0.5f, 0.0f, 0.2f};

float sinewave(float t, float freq) {
    return sin_x87(PI2 * freq * t);
}

float bass(float t, float freq) {
    return sinewave(t + sinewave(t, freq) * 0.015f, freq) *
           envelope(t, &test_env);
}

#define SAMPLEFREQ(freq) (44100.0f / (freq))

// takes number of sample and frequency scale in sample rate
float sqwave(int sample, int samplefreq) {
    return 2.0f * ((sample % samplefreq) > (samplefreq / 2)) - 1.0f;
}

#define NOTE_LENGTH 0.25f
const short g_loop0[] = {
    262, 293, 330, 349, 392, 440, 494, 440, 392, 349, 330, 293,
};

int loopsize = sizeof(g_loop0) / sizeof(short);
/**
 * Generate additional sound as needed.
 * This should manage a synthesizer of some sort.
 */
void generateAudio() {
    int i;
    for(i = 0; i < SND_BUFFER_SAMPLES / 2; i++) {
        // sound data's 16-bit LE stereo (any point in having stereo?)
        short *snd_left  = (short *)g_audioBuffer;
        short *snd_right = (short *)g_audioBuffer + 1;
        float t          = (float)g_audioSample / SND_RATE;
        float sample;
        float notetime = t / NOTE_LENGTH;
        int note       = (int)(notetime) % loopsize;
        short note0    = g_loop0[note];
        int freq       = SAMPLEFREQ(note0);

        sample = sqwave(g_audioSample, freq) *
                 sqwave(g_audioSample, freq * 2) *
                 sqwave(g_audioSample, freq / 2);

        *snd_left  = (sample * SND_GAIN);
        *snd_right = (sample * SND_GAIN);

        g_audioSample++;
        g_audioBuffer++;
    }
}

// ---- framework ----

#ifdef DEBUG
const static char *gl_debugnames[] = {
    "glGetShaderiv", "glGetShaderInfoLog", "glGetProgramiv",
    "glGetProgramInfoLog",
};
funcp glfunc_debug[sizeof(gl_debugnames) / sizeof(char *)];
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)(glfunc_debug[0]))
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)(glfunc_debug[1]))
#define glGetProgramiv ((PFNGLGETSHADERIVPROC)(glfunc_debug[2]))
#define glGetProgramInfoLog ((PFNGLGETSHADERINFOLOGPROC)(glfunc_debug[3]))
#endif
//#define glUniform4fv ((PFNGLUNIFORM4FVPROC)(glfunc[12]))

_inline void initExtensions() {
    int i;
    for(i = 0; i < (sizeof(glnames) / sizeof(char *)); i++) {
        glfunc[i] = (funcp)wglGetProcAddress(glnames[i]);
#ifdef DEBUG
        DEBUG_ASSERT(glfunc[i] != NULL);
#endif
    }
#ifdef DEBUG
    for(i = 0; i < (sizeof(gl_debugnames) / sizeof(char *)); i++) {
        glfunc_debug[i] = (funcp)wglGetProcAddress(gl_debugnames[i]);
        DEBUG_ASSERT(glfunc[i] != NULL);
    }
#endif
}

#ifdef DEBUG
/**
 * Dump any shader error into log output.
 * @note Useful for getting stuff to run on non-Nvidia drivers.
 */
void checkShader(GLuint id) {
    int status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        char buf[8192];
        int len;
        LOG("error when compiling shader:\n");
        glGetShaderInfoLog(id, 8192, &len, buf);
        LOG("%s\n", buf);
        return;
    }
}
#define CHECK_SHADER(id) checkShader(id)
#else
#define CHECK_SHADER(id)
#endif

__declspec(noinline) void buildShaderStage() {
    unsigned shaderId     = glCreateShader(g_compType);
    const char *sources[] = {GLSL_HEADER, g_srcComp};
    glShaderSource(shaderId, 2, sources, NULL);
    glCompileShader(shaderId);
    glAttachShader(g_currentProgram, shaderId);
    CHECK_SHADER(shaderId);
}

/**
 * Build shader using current shader sources.
 */
__declspec(noinline) unsigned buildGLProgram() {
#ifdef DEBUG
    int status;
#endif
    g_currentProgram = glCreateProgram();

    g_compType = GL_VERTEX_SHADER;
    g_srcComp = g_srcVert;
    buildShaderStage();

    g_compType = GL_FRAGMENT_SHADER;
    g_srcComp = g_srcFrag;
    buildShaderStage();

    g_srcComp = g_srcGeom;
    if(g_srcComp) {
        g_compType = GL_GEOMETRY_SHADER;
        buildShaderStage();
    }
    glLinkProgram(g_currentProgram);
    glUseProgram(g_currentProgram);

#ifdef DEBUG
    glGetProgramiv(g_currentProgram, GL_LINK_STATUS, &status);
    if(status != GL_TRUE) {
        char buf[8192];
        int len;
        LOG("error when compiling shader:\n");
        glGetProgramInfoLog(g_currentProgram, 8192, &len, buf);
        LOG("%s\n", buf);
    }
#endif

    // Bind any extra uniforms here, like:
    // glUniform1i(glGetUniformLocation(prog, "tex1"), 1);
    return g_currentProgram;
}

/**
 * Main function, sort of.
 * @note Using WinMainCRTStartup results in a smaller program than the normal
 *       WinMain, but some things may not work as expected. Since we're not
 *       even linking the C runtime, this may not be an issue.
 */
int WINAPI WinMainCRTStartup() {
    PIXELFORMATDESCRIPTOR pfd;
    HDC g_hDC;
    MSG msg;
#ifdef USE_FULLSCREEN
    // Just passing WS_MAXIMIZE would make the window fullscreen, but then
    // we'd have to guess the aspect ratio etc. (not that 16/9 is unlikely)
    g_windowWidth  = GetSystemMetrics(SM_CXSCREEN);
    g_windowHeight = GetSystemMetrics(SM_CYSCREEN);
    g_hWnd = CreateWindowEx(WS_EX_TOPMOST, "edit", 0, WS_POPUP | WS_VISIBLE, 0,
                            0, g_windowWidth, g_windowHeight, 0, 0, 0, 0);
#else
    g_windowWidth  = 640;
    g_windowHeight = 400;
    g_hWnd = CreateWindowEx(WS_EX_TOPMOST, "edit", 0, WS_POPUP | WS_VISIBLE, 0,
                            0, g_windowWidth, g_windowHeight, 0, 0, 0, 0);
#endif
    g_hDC = GetDC(g_hWnd);

    DEBUG_INIT("log.txt");

    // Just assume the memory was zeroed at process launch?
    // memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    // This must be filled or the program may crash.
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);

    pfd.cColorBits = pfd.cDepthBits = 32;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

    SetPixelFormat(g_hDC, ChoosePixelFormat(g_hDC, &pfd), &pfd);
    wglMakeCurrent(g_hDC, wglCreateContext(g_hDC));

#ifdef USE_FULLSCREEN
    ShowCursor(0);
#endif

    // ---- init graphics and stuff
    LOG("preparing shaders\n");
    initExtensions();

    initDemo();

#ifndef BMP_OUT
    LOG("creating audio thread...\n");
    // this way the code doesn't have to be very fast
    CreateThread(0, 0, audioThread, 0, 0, 0);
#endif
    LOG("entering main loop\n");

    do {
        // replacing this with a simple async key state check might shorten
        // the program a bit, but Windows will hang for a while since its
        // event messages are not being received at all
        PeekMessage(&msg, g_hWnd, 0, 0, PM_REMOVE);
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_ESCAPE) {
                break;
            }
        }
        runDemo();
#ifndef BMP_OUT
        glFinish();
        SwapBuffers(g_hDC);
#else
        if(update_interval++ == 10) {
            update_interval = 0;
            glFinish();
            SwapBuffers(g_hDC);
        }
#endif
    } while(g_running);
    DEBUG_CLOSE;

    ExitProcess(0);
}

static LPDIRECTSOUNDBUFFER g_soundBuffer;
char g_nextAudioUpdate = 0; // next buffer half to fill

/**
 * Check if we should update part of the playback buffer or not.
 * Call generateAudio to do so if needed .
 * @return Milliseconds until this really needs to be called again.
 */
int runAudio() {
    unsigned int pos_play, pos_write;
    int *ptr0, *ptr1;
    int len0, len1;

    // Ask the buffer where it's currently playing and where we can write.
    IDirectSoundBuffer8_GetCurrentPosition(g_soundBuffer, &pos_play,
                                           &pos_write);

    if(pos_play > SND_BUFFER_SAMPLES * 2 && g_nextAudioUpdate == 0) {
        IDirectSoundBuffer8_Lock(g_soundBuffer, 0, SND_BUFFER_SAMPLES * 2,
                                 &ptr0, &len0, &ptr1, &len1, 0);
        //
        g_nextAudioUpdate = 1;
    } else if(pos_play < SND_BUFFER_SAMPLES * 2 && g_nextAudioUpdate == 1) {
        // playing the first half of the buffer, second needs update
        IDirectSoundBuffer8_Lock(g_soundBuffer, SND_BUFFER_SAMPLES * 2,
                                 SND_BUFFER_SAMPLES * 2, &ptr0, &len0, &ptr1,
                                 &len1, 0);
        g_nextAudioUpdate = 0; // update first half next
    } else {
        // we weren't ready to update so there must be at least half the buffer
        // duration left until an update is absolutely needed
        return SND_BUFFER_SAMPLES * 250 / SND_RATE;
    }
    // if we got here we must have a lock
    g_audioBuffer = ptr0;
    generateAudio();
    IDirectSoundBuffer8_Unlock(g_soundBuffer, ptr0, len0, ptr1, len1);
    return SND_BUFFER_SAMPLES * 250 / SND_RATE;
}

DWORD WINAPI audioThread(void *param) {
    DSBUFFERDESC buffer_desc;
    WAVEFORMATEX wave_format;
    LPDIRECTSOUND8 ds8;
#ifdef DEBUG
    if(DirectSoundCreate8(NULL, &ds8, NULL) != DS_OK) {
        LOG("DirectSoundCreate8 failed!\n");
    } else {
        LOG("DirectSoundCreate8 ok.\n");
    }
#else
    DirectSoundCreate8(NULL, &ds8, NULL);
#endif
    IDirectSound8_SetCooperativeLevel(ds8, g_hWnd, DSSCL_NORMAL);

    LOG("creating sound buffer\n");
    buffer_desc.dwSize        = sizeof(DSBUFFERDESC);
    buffer_desc.dwFlags       = DSBCAPS_GLOBALFOCUS;
    buffer_desc.dwBufferBytes = SND_BUFFER_SAMPLES * 4;
    buffer_desc.dwReserved    = 0; // needed?

    // All of the macros that can be passed here require linking to some
    // obscure
    // GUID-related file. Let's hope it doesn't break or breaks gracefully.
    // buffer_desc.guid3DAlgorithm = DS3DALG_NO_VIRTUALIZATION;
    buffer_desc.lpwfxFormat = &wave_format;

    wave_format.wFormatTag      = WAVE_FORMAT_PCM;
    wave_format.nChannels       = 2;
    wave_format.nSamplesPerSec  = SND_RATE;
    wave_format.nAvgBytesPerSec = SND_RATE * 4; // rate*block_align
    wave_format.nBlockAlign     = 4;
    wave_format.wBitsPerSample  = 16;
#ifdef DEBUG
    if(IDirectSound8_CreateSoundBuffer(ds8, &buffer_desc, &g_soundBuffer,
                                       NULL) != DS_OK) {
        LOG("IDirectSound8_CreateSoundBuffer failed!\n");
    } else {
        LOG("IDirectSound8_CreateSoundBuffer ok.\n");
    }
#else
    IDirectSound8_CreateSoundBuffer(ds8, &buffer_desc, &g_soundBuffer, NULL);
#endif
    runAudio(); // fill buffer with something
    IDirectSoundBuffer8_Play(g_soundBuffer, 0, 0, DSBPLAY_LOOPING);

    while(g_running) {
        // just busy loop in case of bad scheduling
        runAudio();
        // Sleep(runAudio());
    }
    return 0;
}
