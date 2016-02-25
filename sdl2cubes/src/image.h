#ifndef CUBES_IMAGE_H
#define CUBES_IMAGE_H

// Loads an image with a specified filename and returns the handle of a
// GL texture, or 0 if something goes wrong.
unsigned loadImageToTexture(const char *filename);

#endif
