#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include "shaders.h"

ShaderSourceSpec *shaderSpecs = NULL;

GLuint buildShaderStage(GLenum type, const char *src, const char *id) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if(ok != GL_TRUE) { // fail
        GLint logSize = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
        if(logSize > 0) {
            GLchar *log = (GLchar *)malloc(logSize);
            glGetShaderInfoLog(shader, logSize, NULL, log);
            if(id) {
                fprintf(stderr, "Shader \"%s\" error: %s\n", id, log);
            } else {
                fprintf(stderr, "Shader %p error: %s\n", src, log);
            }
            free(log);
        }
        glDeleteShader(shader);
        return 0; // shader ids are always nonzero so this can signal error
    }
    return shader;
}

GLuint buildShaderStageFromFile(GLenum type, const char *filename) {
    GLuint shader = 0;
    FILE *file = fopen(filename, "rb");
    if(!file) {
        fprintf(stderr, "Unable to read shader: %s\n", filename);
        return 0;
    }
    // figure out true file length without filesystem APIs
    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    // pass file contents for compilation
    char *buf  = (char *)malloc(len + 1);
    size_t got = fread(buf, 1, len, file);
    if(got == len) {
        buf[len] = '\0';
        shader   = buildShaderStage(type, buf, filename);
    } else {
        fprintf(stderr, "Unable to fully read %s!\n", filename);
    }
    free(buf);

    fclose(file);
    return shader;
}

void buildShaderFromSpecs(ShaderSourceSpec *spec) {
    GLuint program = 0, vertex = 0, fragment = 0, geometry = 0;
    GLint ok = 0;

    vertex = buildShaderStageFromFile(GL_VERTEX_SHADER, spec->vertFile);
    if(!vertex) {
        goto exit;
    }
    fragment = buildShaderStageFromFile(GL_FRAGMENT_SHADER, spec->fragFile);
    if(!fragment) {
        goto exit;
    }
    if(spec->geomFile) {
        geometry =
            buildShaderStageFromFile(GL_GEOMETRY_SHADER, spec->geomFile);
        if(!geometry) {
            goto exit;
        }
    }

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    if(geometry) {
        glAttachShader(program, geometry);
    }

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if(ok != GL_TRUE) { // link failed
        GLint logSize = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
        if(logSize > 0) {
            GLchar *log = (GLchar *)malloc(logSize);
            glGetProgramInfoLog(program, logSize, NULL, log);
            fprintf(stderr, "Shader link failed: %s\n", log);
            free(log);
        }
        glDeleteProgram(program);
        program = 0;
    }

exit:
    // Deleting individual stages is ok at this point: they will not be
    // freed by the GL implementation while the programs that use them exist.
    if(vertex) {
        glDeleteShader(vertex);
    }
    if(fragment) {
        glDeleteShader(fragment);
    }
    if(geometry) {
        glDeleteShader(geometry);
    }
    *spec->idPtr = program;
}

void bindUniformBlock(int program, int slot, const char *name) {
    GLuint index = glGetUniformBlockIndex(program, name);
    if(index == GL_INVALID_INDEX) {
        // printf("warning: no uniform block %s in shader program!\n", name);
        return;
    }
    glUniformBlockBinding(program, index, slot);
}

void reloadShaders() {
    // go through the shader source specs and replace the shaders.
    ShaderSourceSpec *spec = shaderSpecs;
    while(spec) {
        if(*spec->idPtr) {
            glDeleteProgram(*spec->idPtr);
            *spec->idPtr = 0;
        }
        buildShaderFromSpecs(spec);
        if(*spec->idPtr) { // did we get a nonzero shader id?
            bindUniformBlock(*spec->idPtr, 0, "FrameParams");
            bindUniformBlock(*spec->idPtr, 1, "ObjectParams");
            // If the shader has a post-compile handler
            // (e.g. for extra bindings), give it a poke now.
            if(spec->postCompile) {
                spec->postCompile(spec);
            }
        }
        spec = (ShaderSourceSpec *)spec->next;
    }
}

void addShaderSource(GLuint *idPtr, const char *vertFile, const char *fragFile,
                     const char *geomFile,
                     void (*postCompile)(ShaderSourceSpec *)) {
    ShaderSourceSpec *spec =
        (ShaderSourceSpec *)malloc(sizeof(ShaderSourceSpec));
    memset(spec, 0, sizeof(ShaderSourceSpec));

    spec->idPtr       = idPtr;
    spec->vertFile    = vertFile;
    spec->fragFile    = fragFile;
    spec->geomFile    = geomFile;
    spec->postCompile = postCompile;

    if(shaderSpecs) {
        spec->next = shaderSpecs;
    }
    shaderSpecs = spec;
}
