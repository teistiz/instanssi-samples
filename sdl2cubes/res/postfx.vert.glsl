#version 330

in vec3 pos;

out vec2 texCoord;

void main() {
    texCoord = pos.xy * 0.5 + vec2(0.5);
    gl_Position = vec4(pos, 1.0);
}
