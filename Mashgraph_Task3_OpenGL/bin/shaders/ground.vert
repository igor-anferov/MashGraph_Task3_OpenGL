#version 330

in vec3 point;
in vec3 normal;
out vec3 point_world;
out vec2 TexCoord;
out vec3 norm;

uniform mat4 camera;

void main() {
    point_world = point;
    point_world.x = point_world.x * 2 - 1;
    point_world.z = point_world.z * 2 - 1;
    gl_Position = camera * vec4(point_world, 1.0f);
    TexCoord = point.xz;
    norm = normal;
}
