#ifndef CUBES_MESH_H
#define CUBES_MESH_H

#include <stdio.h>

// mesh data size counters
typedef struct MeshStats {
    unsigned positions; // # of unique vertex positions
    unsigned normals;   // # of unique normal vectors
    unsigned texcoords; // # of unique texcoords
    unsigned vertices;  // # of vertices
} MeshStats;

// parsed mesh data
typedef struct Mesh {
    MeshStats stats;   // stats about mesh
    float *attribs;    // raw vertex attribute data
    unsigned *indices; // raw index data
} Mesh;

// read mesh from named file
Mesh *meshReadOBJ(const char *filename);
// read mesh from file object, with descriptive filename
Mesh *meshReadOBJF(FILE *file, const char *filename);
// free data associated with a mesh
void meshClose(Mesh *mesh);
// get number of floats in a mesh
unsigned meshGetNumFloats(Mesh *mesh);
// get number of vertices in a mesh
unsigned meshGetNumVertices(Mesh *mesh);
// pack data into target buffer (with space for meshGetNumFloats(mesh) floats)
void meshPackVertices(Mesh *mesh, float *buffer);

#endif