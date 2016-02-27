#ifndef CUBES_SHADERS_H
#define CUBES_SHADERS_H

#include <time.h>

typedef struct ShaderSourceSpec ShaderSourceSpec;

typedef struct ShaderSourceSpec {
    GLuint *idPtr; // address of (global?) id to overwrite
    const char *vtxFile;
    const char *fragFile;
    const char *geomFile;
    time_t vtxTime;
    time_t fragTime;
    time_t geomTime;
    void (*postCompile)(ShaderSourceSpec *);
    ShaderSourceSpec *next;
} ShaderSourceSpec;

GLuint buildShaderStage(GLenum type, const char *src, const char *id);
GLuint buildShaderStageFromFile(GLenum type, const char *filename);
void buildShaderFromSpecs(ShaderSourceSpec *spec);
void reloadShaders();
void addShaderSource(GLuint *idPtr, const char *vertexFile,
                     const char *fragmentFile, const char *geometryFile,
                     void (*postCompile)(ShaderSourceSpec *));

#endif
