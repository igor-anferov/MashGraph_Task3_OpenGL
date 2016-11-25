#version 330 core

in vec3 point;

out vec3 TexCoords;

uniform mat4 camera;

void main() {
    gl_Position = camera * vec4(point, 1.0);
    TexCoords = point;
}
