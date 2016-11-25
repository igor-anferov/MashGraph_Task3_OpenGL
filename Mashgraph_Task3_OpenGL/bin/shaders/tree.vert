#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec2 TexCoords;

uniform mat4 camera;

void main()
{
    float M_PI = 3.1415926535897932384626433832795;
    mat4 scale = mat4(1);
    scale[0][0] = scale[1][1] = scale[2][2] = 0.015;
    mat4 translate = mat4(1);
    translate[3][0] = translate[3][2] = 0;
    float rotationAngle = 2*M_PI/3;
    mat4 rotationY = mat4( cos( rotationAngle ), 0,  -sin( rotationAngle ), 0,
                         0,                    1,  0,                    0,
                         sin( rotationAngle ), 0,  cos( rotationAngle ), 0,
                         0,                    0,  0,                    1
                         );

    gl_Position = camera * translate * rotationY * scale * vec4(position, 1.0f);
    TexCoords = texCoords;
}
