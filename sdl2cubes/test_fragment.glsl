#version 330

uniform sampler2D smpColorTexture;

// From the vertex shader. The per-vertex outputs are
// interpolated to give us per-fragment values here.
in vec2 vertexT;
in vec3 vertexN;

// What goes into render output (blending operations, etc.)
layout(location=0) out vec4 outColor;

void main() {
    outColor = texture(smpColorTexture, vertexT);
}
