#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
in vec3 translation;

out vec2 TexCoords;
out vec3 norm;
out vec3 point_world;

uniform mat4 camera;
uniform float time;

void main()
{
    float M_PI = 3.1415926535897932384626433832795;
    mat4 scale = mat4(1);
    scale[0][0] = scale[1][1] = scale[2][2] = 0.001;
    mat4 translate = mat4(1);
    translate[3][0] = translation.x * 2 - 1;
    translate[3][1] = translation.y;
    translate[3][2] = translation.z * 2 - 1;
    
    float rotationAngle = gl_InstanceID;
    mat4 rotationY = mat4( cos( rotationAngle ), 0,  -sin( rotationAngle ), 0,
                          0,                    1,  0,                    0,
                          sin( rotationAngle ), 0,  cos( rotationAngle ), 0,
                          0,                    0,  0,                    1
                          );
    
    float tiltAngle = (sin(-10 * translation.x + time)+1)/2*(M_PI/12);
    float a = tiltAngle * sqrt((position.y)) * 0.1;
    mat4 tiltMatrix       = mat4( cos(a), -sin(a), 0, 0,
                                 sin(a),  cos(a), 0, 0,
                                 0,       0,      1, 0,
                                 0,       0,      0, 1
                                 );
    
    point_world = vec3(translate * tiltMatrix * rotationY * scale * vec4(position, 1.0f));
    norm = normalize(vec3(translate * tiltMatrix * rotationY * vec4(normal,1)));
    gl_Position = camera * translate * tiltMatrix * rotationY * scale * vec4(position, 1.0f);
    TexCoords = texCoords;
}
