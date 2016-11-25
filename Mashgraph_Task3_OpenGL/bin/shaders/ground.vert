#version 330

in vec3 point;
in vec3 normal;
out vec3 point_world;
out vec2 TexCoord;
out vec3 norm;

uniform mat4 camera;

void main() {
    gl_Position = camera * vec4(point, 1.0f);
    TexCoord = point.xz;
    point_world = point;
    norm = normal;
}
