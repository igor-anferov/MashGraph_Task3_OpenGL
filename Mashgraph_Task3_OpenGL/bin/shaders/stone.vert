#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec2 TexCoords;
out vec3 norm;
out vec3 point_world;

uniform vec3 translation;
uniform mat4 camera;

void main()
{
    float M_PI = 3.1415926535897932384626433832795;
    mat4 scale = mat4(1);
    scale[0][0] = scale[1][1] = scale[2][2] = 0.05;
    mat4 translate = mat4(1);
    translate[3][0] = translation.x;
    translate[3][1] = translation.y;
    translate[3][2] = translation.z;
    float rotationAngle = 0;
    mat4 rotationY = mat4( cos( rotationAngle ), 0,  -sin( rotationAngle ), 0,
                          0,                    1,  0,                    0,
                          sin( rotationAngle ), 0,  cos( rotationAngle ), 0,
                          0,                    0,  0,                    1
                          );
    
    point_world = vec3(translate * rotationY * scale * vec4(position, 1.0f));
    norm = normalize(vec3(translate * rotationY * vec4(normal,1)));
    gl_Position = camera * translate * rotationY * scale * vec4(position, 1.0f);
    TexCoords = texCoords;
}
