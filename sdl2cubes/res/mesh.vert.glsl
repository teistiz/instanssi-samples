#version 330

layout(std140) uniform FrameParams {
    mat4 projection;    // viewspace to screenspace
    float time;
};

layout(std140) uniform ObjectParams {
    mat4 transform;     // object to viewspace
};

// Declare vertex format.
// This should match what was done with glVertexAttribPointer.
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTex;
layout(location=2) in vec3 aNormal;

// This is passed to next shader stage.
out vec2 vertexT;
out vec3 vertexN;

void main() {
    // flip texture so we don't have to do it in C.
    vertexT = aTex * vec2(1.0, -1.0);
    // Cutting the matrix down to mat3 removes the translation,
    // but if the view matrix shears or otherwise mangles coordinates
    // beyond translating, scaling and rotating this may be wrong.
    vertexN = normalize(mat3(transform) * aNormal);
    // just pass the vertices to rasterization as-is
    //gl_Position = projection * vec4(aPos, 1.0);
    gl_Position = projection * (transform * vec4(aPos, 1.0));
}
