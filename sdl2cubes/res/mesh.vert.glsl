#version 330

layout(std140) uniform FrameParams {
    mat4 projection;    // viewspace to projection/clip space
    float time;
};

layout(std140) uniform ObjectParams {
    mat4 transform;     // object to viewspace
};

// Declare vertex attribute inputs.
// This should match what was done with glVertexAttribPointer.
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTex;
layout(location=2) in vec3 aNormal;

// These are passed to next shader stage.
out vec2 vertexT;
out vec3 vertexN;

void main() {
    // flip texture so we don't have to do it in C.
    vertexT = aTex * vec2(1.0, -1.0);
    // Cutting the matrix down like this removes the translation,
    // but if the modelview matrix shears or otherwise mangles coordinates
    // beyond translating, uniform scaling and rotating this may be wrong.
    // The correct normal/rotation-only transform would be the transpose of
    // the inverted transform matrix, but that's a bit expensive to do here.
    vertexN = normalize(mat3(transform) * aNormal);
    // Apply transformations to project mesh-space vertex positions to screen.
    gl_Position = projection * (transform * vec4(aPos, 1.0));
}
