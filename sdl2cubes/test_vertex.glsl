#version 330

 // this is the vertex pos input by default
layout(location=0) in vec3 pos;

// This is passed to next shader stage.
out vec2 texCoord;

void main() {
    // map vertex pos [-1..1] ranges to [0..1]
    texCoord = vec2(pos.xy) * 0.5 + 0.5;
    // also, flip the texture since GL texcoord (0, 0) is bottom left
    texCoord.y = 1.0 - texCoord.y;
    // just pass the vertices to rasterization as-is
    gl_Position = vec4(pos, 1.0);
}
