#version 330

in vec2 point;
in vec3 center;
in float radius;
out vec2 points;
out vec3 norm;
out vec3 point_world;
out float instID;

uniform mat4 camera;
uniform float time;

void main() {
    instID = gl_InstanceID;
    float M_PI = 3.1415926535897932384626433832795;
    norm = vec3(0, 0, 1);
    float wingAngle = (sin(time+point.x*point.x*10)+1)/2*(M_PI/2) * point.x;
//    wingAngle = M_PI/4 * point.x;
    mat4 wingMatrix      = mat4( cos( wingAngle ), 0,  sin( wingAngle ), 0,
                                0,                    1,  0,                    0,
                                -sin( wingAngle ), 0,  cos( wingAngle ), 0,
                                0,                    0,  0,                    1
                                );
    float a = M_PI/3;
    mat4 rotateX = mat4( 1, 0,       0,      0,
                         0, cos(a), -sin(a), 0,
                         0, sin(a),  cos(a), 0,
                         0, 0,       0,      1
                       );
    a = 0;
    mat4 rotateZ = mat4( cos(a), -sin(a), 0, 0,
                         sin(a),  cos(a), 0, 0,
                         0,       0,      1, 0,
                         0,       0,      0, 1
                       );
    a = 3*M_PI/2;
    mat4 rotateY = mat4( cos(a), 0,  sin(a), 0,
                         0,      1,  0,      0,
                        -sin(a), 0,  cos(a), 0,
                         0,      0,  0,      1
                       );
    a = time/100/radius;
    mat4 rotation = mat4( cos(a), 0,  sin(a), 0,
                          0,      1,  0,      0,
                         -sin(a), 0,  cos(a), 0,
                          0,      0,  0,      1
                        );
    
    points = vec2(1) - point;
    mat4 scaleMatrix = mat4(1.0);
    scaleMatrix[0][0] = 0.125e-1;
    scaleMatrix[1][1] = 0.25e-1;
    mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = 0;
    positionMatrix[3][1] = (sin(time+point.x*point.x*10)+1)/2*0.01 + (sin(time/100)+1)/2*0.05;
    positionMatrix[3][2] = radius;
    mat4 moveMatrix = mat4(1.0);
    moveMatrix[3][0] = center.x;
    moveMatrix[3][1] = center.y;
    moveMatrix[3][2] = center.z;

    norm = vec3(moveMatrix * rotation * positionMatrix * rotateY * rotateX * rotateZ * wingMatrix * vec4(norm, 1));
    point_world = vec3(moveMatrix * rotation * positionMatrix * rotateY * rotateX * rotateZ * wingMatrix * scaleMatrix * vec4(point,0,1));
    gl_Position = camera * moveMatrix * rotation * positionMatrix * rotateY * rotateX * rotateZ * wingMatrix * scaleMatrix * vec4(point,0,1);
}
