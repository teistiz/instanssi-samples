#version 330

layout(std140) uniform FrameParams {
    mat4 projection;
    float time;
};

uniform sampler2D smpImage;

in vec2 texCoord;

out vec4 outColor;

void main() {
    outColor = texture(smpImage, texCoord);
}
