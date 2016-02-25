#version 330

uniform sampler2D smpColorTexture;

in vec2 texCoord;

out vec4 outColor;

void main() {
    outColor = texture(smpColorTexture, texCoord);
}
