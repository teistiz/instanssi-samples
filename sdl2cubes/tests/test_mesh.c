#include "../src/mesh_obj.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *args[]) {
    if(argc < 2) {
        printf("test_mesh test-mesh-name.obj\n");
        return 0;
    }
    // read mesh from standard input, push into obj parser
    Mesh *mesh = meshReadOBJ(args[1]);
    if(!mesh) {
        return 0;
    }

    unsigned numFloats = meshGetNumFloats(mesh);
    unsigned numVertices = meshGetNumVertices(mesh);

    float *buf = malloc(sizeof(float) * numFloats);
    meshPackVertices(mesh, buf);

    meshClose(mesh);
    return 0;
}
