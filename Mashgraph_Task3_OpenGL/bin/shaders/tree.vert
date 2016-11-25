#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec2 TexCoords;

uniform mat4 camera;

void main()
{
    mat4 scale = mat4(1);
    scale[0][0] = scale[1][1] = scale[2][2] = 0.015;
    mat4 translate = mat4(1);
    translate[3][0] = translate[3][2] = 0.5;
    mat4 rotationY = mat4(1);
    rotationY[0][0] = rotationY[2][2] = -1;
    gl_Position = camera * translate * rotationY * scale * vec4(position, 1.0f);
    TexCoords = texCoords;
}
