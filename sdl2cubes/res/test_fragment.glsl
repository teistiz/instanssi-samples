#version 330

uniform sampler2D smpColorTexture;

// From the vertex shader. The per-vertex outputs are
// interpolated to give us per-fragment values here.
in vec2 vertexT;
in vec3 vertexN;

// What goes into render output (blending operations, etc.)
layout(location=0) out vec4 outColor;

void main() {
    vec3 normal = normalize(vertexN);
    float light = max(0.0, dot(vec3(0, 1, 0), normal));
    light += 0.2;
    outColor = texture(smpColorTexture, vertexT) * light;
}
