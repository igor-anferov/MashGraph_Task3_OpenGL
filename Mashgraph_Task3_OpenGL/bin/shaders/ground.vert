#version 330

layout (location = 0) in vec2 point;
out vec2 TexCoord;

uniform mat4 camera;

void main() {
    gl_Position = camera * vec4(point.x, 0.0f, point.y, 1.0f);
    TexCoord = point;
}
