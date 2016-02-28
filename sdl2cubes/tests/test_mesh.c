#include "../src/mesh_obj.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *args[]) {
    if(argc < 2) {
        printf("usage: test_mesh filename.obj\n");
        printf("  pass - (a dash character) as filename to read from stdin\n");
        return 0;
    }
    Mesh *mesh;
    // read mesh from standard input or file, push into obj parser
    if(!strcmp("-", args[1])) {
        mesh = meshReadOBJF(stdin, "(stdin)");
    } else {
        mesh = meshReadOBJ(args[1]);
    }
    if(!mesh) {
        return 0;
    }
    unsigned numFloats   = meshGetNumFloats(mesh);
    unsigned numVertices = meshGetNumVertices(mesh);

    float *buf = malloc(sizeof(float) * numFloats);
    meshPackVertices(mesh, buf);
    meshClose(mesh);
    free(buf);
    return 0;
}
