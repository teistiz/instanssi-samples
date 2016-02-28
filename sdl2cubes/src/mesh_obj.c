/**
 * mesh_obj.c
 * Rough OBJ mesh loader. Can read vertex positions, normals and texture coords
 * and pack them into memory in a GPU-friendly format.
 * Ignores materials as they aren't very useful with custom shaders.
 */

#include "mesh_obj.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int countFaceVertices(const char *line);
static int startsWith(const char *str, const char *token);
static int tallyOBJFile(FILE *file, MeshStats *stats);
static int loadOBJFile(FILE *file, Mesh *mesh);

static float *meshGetVertexPtr(Mesh *mesh);
static float *meshGetNormalPtr(Mesh *mesh);
static float *meshGetTexPtr(Mesh *mesh);
static unsigned *meshGetIndexPtr(Mesh *mesh);

// This is a scanf pattern for recognizing faces.
// scanf should return 9 for triangles and 12 for quads.
const char FACE_PATTERN[] = "f %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u";

// read .obj mesh from file with descriptive name for errors
// Makes exactly one allocation.
Mesh *meshReadOBJInternal(FILE *file, const char *filename) {
    MeshStats stats;
    // just parse the file twice, it's easier with formats like this
    if(!tallyOBJFile(file, &stats)) {
        fprintf(stderr, "error: unable to parse OBJ file %s\n", filename);
        return NULL;
    }
    // calculate storage for mesh contents
    // positions and normals are 3 float32s each, texcoords are 2
    size_t attribBytes =
        4 * (stats.positions * 3 + stats.normals * 3 + stats.texcoords * 2);
    // each vertex has separate indices for position, normal and texture coord
    size_t indexBytes = sizeof(int) * 3 * stats.vertices;

    Mesh *mesh  = (Mesh *)malloc(sizeof(Mesh) + attribBytes + indexBytes);
    mesh->stats = stats;
    // attribute data comes right after the Mesh struct
    mesh->attribs = (float *)((char *)mesh + sizeof(Mesh));
    // indices after the attribute data
    mesh->indices = (unsigned *)((char *)mesh->attribs + attribBytes);

    if(!loadOBJFile(file, mesh)) {
        fprintf(stderr, "error: unable to parse OBJ file %s\n", filename);
        free(mesh);
        mesh = NULL;
    }

    return mesh;
}

Mesh *meshReadOBJ(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if(!file) {
        fprintf(stderr, "error: unable to open OBJ file %s\n", filename);
        return NULL;
    }
    Mesh *mesh = meshReadOBJInternal(file, filename);
    fclose(file);
    return mesh;
}

Mesh *meshReadOBJF(FILE *file, const char *filename) {
    return meshReadOBJInternal(file, filename);
}

void meshClose(Mesh *mesh) { free(mesh); }

unsigned meshGetNumVertices(Mesh *mesh) { return mesh->stats.vertices; }
unsigned meshGetNumFloats(Mesh *mesh) {
    // each vertex is made of a (vec3 pos, vec2 tex, vec3 normal)
    return meshGetNumVertices(mesh) * (3 + 2 + 3);
}
unsigned *meshGetIndexPtr(Mesh *mesh) { return mesh->indices; }
float *meshGetVertexPtr(Mesh *mesh) { return mesh->attribs; }
float *meshGetTexPtr(Mesh *mesh) {
    return meshGetVertexPtr(mesh) + 3 * mesh->stats.positions;
}
float *meshGetNormalPtr(Mesh *mesh) {
    return meshGetTexPtr(mesh) + 2 * mesh->stats.texcoords;
}

unsigned emitTriangle(unsigned *pIndices, int v0, int t0, int n0, int v1,
                      int t1, int n1, int v2, int t2, int n2) {
    *(pIndices++) = v0 - 1;
    *(pIndices++) = t0 - 1;
    *(pIndices++) = n0 - 1;
    *(pIndices++) = v1 - 1;
    *(pIndices++) = t1 - 1;
    *(pIndices++) = n1 - 1;
    *(pIndices++) = v2 - 1;
    *(pIndices++) = t2 - 1;
    *(pIndices++) = n2 - 1;
    return 9;
}

#define IN_RANGE(p, min, max) (p >= min && p <= max)

int sanityCheck(Mesh *mesh, unsigned v0, unsigned t0, unsigned n0, unsigned v1,
                unsigned t1, unsigned n1, unsigned v2, unsigned t2,
                unsigned n2) {
    MeshStats *stats = &mesh->stats;
    int ok = 1;
    ok &= IN_RANGE(v0, 1, stats->positions);
    ok &= IN_RANGE(v1, 1, stats->positions);
    ok &= IN_RANGE(v2, 1, stats->positions);
    ok &= IN_RANGE(n0, 1, stats->normals);
    ok &= IN_RANGE(n1, 1, stats->normals);
    ok &= IN_RANGE(n2, 1, stats->normals);
    ok &= IN_RANGE(t0, 1, stats->texcoords);
    ok &= IN_RANGE(t1, 1, stats->texcoords);
    ok &= IN_RANGE(t2, 1, stats->texcoords);
    return 1;
}

int loadOBJFile(FILE *file, Mesh *mesh) {
    // Just collect all the attribs and indices.
    // Converting OBJ indexing into GL-compatible stuff can follow when we
    // are packing attributes into a buffer.
    char *res = NULL;
    float x, y, z;
    unsigned indexOfs  = 0;
    unsigned vertexOfs = 0;
    unsigned texOfs    = 0;
    unsigned normalOfs = 0;

    float *pVertices   = meshGetVertexPtr(mesh);
    float *pTexCoords  = meshGetTexPtr(mesh);
    float *pNormals    = meshGetNormalPtr(mesh);
    unsigned *pIndices = meshGetIndexPtr(mesh);

    char line[256];
    unsigned linenum = 0;

    fseek(file, 0, SEEK_SET);

    while((res = fgets(line, sizeof(line), file))) {
        linenum++;
        if(startsWith(line, "v ")) {
            x = y = z = 0.0f;
            if(sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                pVertices[vertexOfs++] = x;
                pVertices[vertexOfs++] = y;
                pVertices[vertexOfs++] = z;
            } else {
                return 0;
            }
        } else if(startsWith(line, "vn ")) {
            x = y = z = 0.0f;
            if(sscanf(line, "vn %f %f %f", &x, &y, &z) == 3) {
                pNormals[normalOfs++] = x;
                pNormals[normalOfs++] = y;
                pNormals[normalOfs++] = z;
            } else {
                return 0;
            }
        } else if(startsWith(line, "vt ")) {
            x = y = 0.0f;
            if(sscanf(line, "vt %f %f", &x, &y) == 2) {
                pTexCoords[texOfs++] = x;
                pTexCoords[texOfs++] = y;
            } else {
                return 0;
            }
        } else if(startsWith(line, "f ")) {
            // the indices start from 1, so let's fix that here
            unsigned v0, v1, v2, v3;
            unsigned n0, n1, n2, n3;
            unsigned t0, t1, t2, t3;
            int count = sscanf(line, FACE_PATTERN, &v0, &t0, &n0, &v1, &t1,
                               &n1, &v2, &t2, &n2, &v3, &t3, &n3);
            if(!sanityCheck(mesh, v0, t0, n0, v1, t1, n1, v2, t2, n2)) {
                fprintf(stderr, "insane index value on line %u, aborting.\n",
                        linenum);
                return 0;
            }
            if(count == 9) {
                indexOfs += emitTriangle(pIndices + indexOfs, v0, t0, n0, v1,
                                         t1, n1, v2, t2, n2);
            } else if(count == 12) {
                if(!sanityCheck(mesh, v0, t0, n0, v2, t2, n2, v3, t3, n3)) {
                    fprintf(stderr,
                            "insane index value on line %u, aborting.\n",
                            linenum);
                    return 0;
                }
                // split quad into two triangles
                indexOfs += emitTriangle(pIndices + indexOfs, v0, t0, n0, v1,
                                         t1, n1, v2, t2, n2);
                indexOfs += emitTriangle(pIndices + indexOfs, v0, t0, n0, v2,
                                         t2, n2, v3, t3, n3);
            } else {
                return 0;
            }
        }
    }
    return 1;
}

void meshOutput(Mesh *mesh, FILE *file) {
    MeshStats *stats = &mesh->stats;

    unsigned *pIndices = meshGetIndexPtr(mesh);
    float *pVertices   = meshGetVertexPtr(mesh);
    float *pTexCoords  = meshGetTexPtr(mesh);
    float *pNormals    = meshGetNormalPtr(mesh);

    for(unsigned i = 0; i < stats->positions; i++) {
        fprintf(file, "v %f %f %f\n", pVertices[i * 3], pVertices[i * 3 + 1],
                pVertices[i * 3 + 2]);
    }
    for(unsigned i = 0; i < stats->texcoords; i++) {
        fprintf(file, "vt %f %f\n", pTexCoords[i * 2], pTexCoords[i * 2 + 1]);
    }
    for(unsigned i = 0; i < stats->normals; i++) {
        fprintf(file, "vn %f %f %f\n", pNormals[i * 3], pNormals[i * 3 + 1],
                pNormals[i * 3 + 2]);
    }

    for(unsigned i = 0; i < stats->vertices; i += 3) {
        unsigned *f = &pIndices[i * 3];
        fprintf(file, "f %u/%u/%u %u/%u/%u %u/%u/%u\n", f[0] + 1, f[1] + 1,
                f[2] + 1, f[3] + 1, f[4] + 1, f[5] + 1, f[6] + 1, f[7] + 1,
                f[8] + 1);
    }
}

void meshPackVertices(Mesh *mesh, float *buffer) {
    MeshStats *stats = &mesh->stats;

    unsigned *pIndices = meshGetIndexPtr(mesh);
    float *pVertices   = meshGetVertexPtr(mesh);
    float *pTexCoords  = meshGetTexPtr(mesh);
    float *pNormals    = meshGetNormalPtr(mesh);

    // at this point the indices should have been validated
    for(unsigned i = 0; i < stats->vertices; i++) {
        unsigned iv = pIndices[i * 3];
        unsigned it = pIndices[i * 3 + 1];
        unsigned in = pIndices[i * 3 + 2];

        buffer[0] = pVertices[iv * 3 + 0];
        buffer[1] = pVertices[iv * 3 + 1];
        buffer[2] = pVertices[iv * 3 + 2];
        buffer[3] = pTexCoords[it * 2 + 0];
        buffer[4] = pTexCoords[it * 2 + 1];
        buffer[5] = pNormals[in * 3 + 0];
        buffer[6] = pNormals[in * 3 + 1];
        buffer[7] = pNormals[in * 3 + 2];
        buffer += 8;
    }
}

int countFaceVertices(const char *line) {
    unsigned a, *f = &a;

    int res = sscanf(line, FACE_PATTERN, f, f, f, f, f, f, f, f, f, f, f, f);
    return res / 3;
}

int startsWith(const char *str, const char *token) {
    return strstr(str, token) == str;
}

int tallyOBJFile(FILE *file, MeshStats *stats) {
    char line[256];
    unsigned n = 1;
    char *res = NULL;
    memset(stats, 0, sizeof(MeshStats));
    fseek(file, 0, SEEK_SET);
    while((res = fgets(line, sizeof(line), file))) {
        if(startsWith(line, "v ")) {
            stats->positions++;
        } else if(startsWith(line, "vn ")) {
            stats->normals++;
        } else if(startsWith(line, "vt ")) {
            stats->texcoords++;
        } else if(startsWith(line, "f ")) {
            int numVerts = countFaceVertices(line);
            if(numVerts == 3) {
                stats->vertices += 3;
            } else if(numVerts == 4) {
                stats->vertices += 6; // we can split the quad later
            } else {
                fprintf(stderr, "error: couldn't parse face on line %u\n", n);
                return 0;
            }
        }
        n++;
    }
    return 1;
}
