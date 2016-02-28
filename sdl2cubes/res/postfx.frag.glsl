#version 330

layout(std140) uniform FrameParams {
    mat4 projection;
    float time;
};

uniform sampler2D smpImage;

in vec2 texCoord;

out vec4 outColor;

float rand(vec2 co) {
    return fract(sin(dot(co.xy + vec2(time), vec2(12.9898, 78.233)))
                 * 43758.5453);
}

void main() {
    vec2 coord = texCoord;
    float offset = rand(gl_FragCoord.yy);
    offset *= offset;
    coord.x += offset * 0.003;

    float lines = mod(gl_FragCoord.y, 2.0) + 0.5;

    outColor = texture(smpImage, coord) * lines;
}
