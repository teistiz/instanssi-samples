#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include <stb_image.h>
#define GLEW_STATIC
#include <GL/glew.h>


unsigned loadImageToTexture(const char *filename) {
    GLuint tex = 0;

    int width, height, comps;
    unsigned char *image = stbi_load(filename, &width, &height, &comps, 0);

    if(!image) {
        fprintf(stderr, "error: can't load image: %s\n", filename);
        return 0;
    }

    GLenum format, internalFormat;
    // This doesn't come close to covering all possible formats,
    // but is good enough for our test files.
    if(comps == 4) {
        internalFormat = GL_RGBA;
        format = GL_RGBA;
    } else if(comps == 3) {
        internalFormat = GL_RGB;
        format = GL_RGB;
    } else {
        // we could handle palette textures at least...
        fprintf(stderr, "error: unsupported pixel format: %s\n", filename);
        goto exit;
    }

    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height,
                 0, format, GL_UNSIGNED_BYTE, image);

    // if we need mips, call glGenerateMipmaps and set this to MIPMAP_LINEAR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

exit:
    stbi_image_free(image);
    return tex;
}
