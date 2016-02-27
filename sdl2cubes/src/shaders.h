#ifndef CUBES_SHADERS_H
#define CUBES_SHADERS_H

typedef struct ShaderSourceSpec ShaderSourceSpec;

typedef struct ShaderSourceSpec {
    GLuint *idPtr; // address of (global?) id to overwrite
    const char *vertFile;
    const char *fragFile;
    const char *geomFile;
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
