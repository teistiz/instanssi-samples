#version 330

uniform sampler2D smpImage;

in vec2 texCoord;

out vec4 outColor;

void main() {
    outColor = texture(smpImage, texCoord);
}
