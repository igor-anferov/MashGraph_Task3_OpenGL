#version 330

in vec3 point;
out vec2 TexCoord;

uniform mat4 camera;

void main() {
    gl_Position = camera * vec4(point, 1.0f);
    TexCoord = point.xz;
}
