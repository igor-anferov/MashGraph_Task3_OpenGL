#version 330

in vec2 points;
out vec4 outColor;
uniform sampler2D ourTexture;

void main() {
    outColor = texture(ourTexture, points);
    if(outColor.a < 0.8)
        discard;
}
