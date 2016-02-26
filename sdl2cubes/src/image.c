#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define GLEW_STATIC
#include <GL/glew.h>


unsigned loadImageToTexture(const char *filename) {
    GLuint tex = 0;
    GLenum format, internalFormat;
    int width = 0, height = 0, components = 0;

    // This can load many image formats, including PNG and JPEG.
    unsigned char *image = stbi_load(filename, &width, &height, &components, 0);

    if(!image) {
        fprintf(stderr, "error: can't open image: %s\n", filename);
        return 0;
    }

    // This doesn't come close to covering all possible formats,
    // but is good enough for our test files.
    // We could pass nonzero components to stbi_load to request a specific
    // pixel format if we really wanted to.
    if(components == 4) {
        internalFormat = format = GL_RGBA;
    } else if(components == 3) {
        internalFormat = format = GL_RGB;
    } else {
        fprintf(stderr, "error: unsupported pixel format: %s\n", filename);
        goto exit;
    }

    glGenTextures(1, &tex);
    // let's at least be consistent about which slot gets side effects
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    // allocate and initialize the texture with our image data
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height,
                 0, format, GL_UNSIGNED_BYTE, image);

    // GL_TEXTURE_MIN_FILTER is set to use mipmaps by default.
    // Unless they're provided or generated, the texture is incomplete
    // and can't be used. If we need mipmaps, call glGenerateMipmaps first
    // and set this to GL_LINEAR_MIPMAP_LINEAR instead.
    // https://www.opengl.org/sdk/docs/man/html/glTexParameter.xhtml
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

exit:
    stbi_image_free(image);
    return tex;
}
