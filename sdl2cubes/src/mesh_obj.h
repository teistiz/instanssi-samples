#ifndef CUBES_MESH_H
#define CUBES_MESH_H

typedef struct MeshStats {
    unsigned positions, normals, texcoords, vertices;
} MeshStats;

typedef struct Mesh {
    MeshStats stats;
    // Holds mesh attributes in float32 format.
    // Vertices first, then texcoords, then normals.
    float *attribs;
    // Triangle indices.
    unsigned *indices;
} Mesh;

// Reads a mesh from a file. May return NULL on failure.
Mesh *meshReadOBJ(const char *filename);
// Frees any data associated with a mesh.
void meshClose(Mesh *mesh);
// Returns the number of floats the mesh data contains.
unsigned meshGetNumFloats(Mesh *mesh);
unsigned meshGetNumVertices(Mesh *mesh);
// Tries to pack the mesh's attrib data into the target location.
void meshPackVertices(Mesh *mesh, float *buffer);

#endif