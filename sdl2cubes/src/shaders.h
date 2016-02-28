#ifndef CUBES_SHADERS_H
#define CUBES_SHADERS_H

typedef struct ShaderSourceSpec ShaderSourceSpec;

// Holds information about registered shader sources.
typedef struct ShaderSourceSpec {
    GLuint *idPtr;        // address of GL id to overwrite
    const char *vertFile; // vertex source file name
    const char *fragFile; // fragment source file name
    const char *geomFile; // geometry source file name
    // handler to call after (re)compilation
    void (*postCompile)(ShaderSourceSpec *);
    // next shader source spec in the list
    ShaderSourceSpec *next;
} ShaderSourceSpec;

GLuint buildShaderStage(GLenum type, const char *src, const char *id);
GLuint buildShaderStageFromFile(GLenum type, const char *filename);

// reload and recompile registered shaders
void reloadShaders();
// register shader source so it can be automatically reloaded
void addShaderSource(GLuint *idPtr, const char *vertexFile,
                     const char *fragmentFile, const char *geometryFile,
                     void (*postCompile)(ShaderSourceSpec *));

#endif
