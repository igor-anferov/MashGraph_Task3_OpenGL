#version 330 core

in vec3 TexCoords;

out vec4 color;

uniform samplerCube skybox;

void main() {
    if (TexCoords.y < -0.2) {
        discard;
    }
    color = texture(skybox, TexCoords);
}
