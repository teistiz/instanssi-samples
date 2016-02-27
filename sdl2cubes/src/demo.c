#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <math.h>   // cosf, sinf
#include <stdio.h>  // printf
#include <string.h> // memset
#include <assert.h>
#include "main.h"
#include "image.h"
#include "shaders.h"
#include "audio.h"
#include "mesh_obj.h"
#include "transform.h"

const char *WINDOW_TITLE = "cubes!?";

float g_time = 0;

// This can be passed to shaders as-is, as long as it matches GLSL's layout
// rules. Basically things are aligned to their size, except that
// vec3 aligns like vec4 and matrices align to their column size
// (vec2/vec3/vec4). Arrays align to their elements' size (iirc).
// See https://www.opengl.org/registry/doc/glspec45.core.pdf#page=159 .
typedef struct MeshUniforms {
    Transform projection;
    Transform view;
} MeshUniforms;

Transform g_tfProjection;
TransformStack *g_tfsView;

typedef struct RenderMesh {
    GLuint vertexArray; // vertex array object id
    GLuint buffer;      // buffer object id
    unsigned vertices;  // number of vertices
} RenderMesh;

GLuint g_vaQuad    = 0; // vertex array for the quad
GLuint g_bufQuad   = 0; // data buffer for the quad
GLuint g_ubGlobals = 0; // uniform buffer for shader params

GLuint g_texTest    = 0; // test texture
GLuint g_texAtlas   = 0; // overlay graphics
GLuint g_shaderMesh = 0; // shader for simple meshes
RenderMesh g_meshSphere; // mesh object

void loadMesh(RenderMesh *dest, const char *filename);
void loadTexture(GLuint *dest, const char *filename);

void initBuffers();
void drawQuad();

// demo init code goes here.
int initDemo() {
    setWindowTitle(WINDOW_TITLE);
    // This starts playing when init has finished.
    setSoundtrack("res/track.ogg");

    g_tfProjection = tfIdentity();
    tfsCreate(&g_tfsView, 64);

    loadMesh(&g_meshSphere, "res/unitcube.obj");
    loadTexture(&g_texTest, "res/quality_graphics.png");

    addShaderSource(&g_shaderMesh, "res/mesh.vert.glsl", "res/mesh.frag.glsl",
                    NULL, NULL);
    addShaderSource(&g_shaderMesh, "res/postfx.vert.glsl",
                    "res/postfx.frag.glsl", NULL, NULL);

    reloadShaders();
    initBuffers();
    return 1;
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

    glUseProgram(g_shaderMesh);

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
    glBindTexture(GL_TEXTURE_2D, g_texTest);

    // this restores the vertex array bindings we made earlier
    glBindVertexArray(g_meshSphere.vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, g_meshSphere.vertices);

    // show what we just drew
    presentWindow();
    return 1;
}

void drawQuad() {
    glBindVertexArray(g_vaQuad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void initBuffers() {
    // Make a buffer for shader uniform parameters.
    // Get id for the new buffer.
    glGenBuffers(1, &g_ubGlobals);
    // Select the new buffer as our current GL_UNIFORM_BUFFER.
    // Technically we could bind it in any buffer slot for now, but GL
    // implementations are allowed to optimize based on where the buffer was
    // bound first. It's probably a good idea to initialize buffers in the slot
    // they will be used in.
    glBindBuffer(GL_UNIFORM_BUFFER, g_ubGlobals);
    // GL_DYNAMIC_DRAW hints the buffer is for drawing with frequent updates
    // See: https://www.opengl.org/sdk/docs/man/html/glBufferData.xhtml .
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MeshUniforms), NULL,
                 GL_DYNAMIC_DRAW);

    // This can be used as a GL_TRIANGLE_FAN of vec2s to draw a rectangle.
    float quadVertices[] = {
        1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
    };
    glGenBuffers(1, &g_bufQuad);
    glBindBuffer(GL_ARRAY_BUFFER, g_bufQuad);
    // Allocate and fill the buffer with data.
    // GL_STATIC_DRAW hints this is static (e.g. mesh) data for drawing.
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

RenderMesh loadMeshToArray(const char *filename);
void loadMesh(RenderMesh *dest, const char *filename) {
    // TODO: Implement like the shader one
    *dest = loadMeshToArray(filename);
}
void loadTexture(GLuint *dest, const char *filename) {
    *dest = loadImageToTexture(filename);
}

// ---- potentially buggy linear algebra follows ----

void tfsClear(TransformStack *tfs) {
    tfs->t[0] = tfIdentity();
    tfs->pos  = 0;
}

void tfsCreate(TransformStack **p, unsigned maxSize) {
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
