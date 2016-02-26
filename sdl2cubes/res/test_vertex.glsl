#version 330

layout(std140) uniform MeshUniforms {
    mat4 projection;
    mat4 view;
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
    vertexT = aTex;
    vertexN = aNormal;
    // just pass the vertices to rasterization as-is
    gl_Position = vec4(aPos, 1.0);
}
