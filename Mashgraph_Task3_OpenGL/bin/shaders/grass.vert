#version 330

in vec4 point;
in vec3 position;
in float rotationAngle;
in float size;
in vec2 randomTiltAngle;
out vec2 points;
out vec3 norm;
out vec3 point_world;

uniform mat4 camera;
uniform float time;

void main() {
    float M_PI = 3.1415926535897932384626433832795;
    norm = vec3(0, 0, 1);
    float tiltAngle = (sin(-10 * position.x + time)+1)/2*(M_PI/12);
    float a = tiltAngle * (point.y)*(point.y);
    mat4 tiltMatrix       = mat4( cos(a), -sin(a), 0, 0,
                                  sin(a),  cos(a), 0, 0,
                                  0,       0,      1, 0,
                                  0,       0,      0, 1
                                );
    a = randomTiltAngle.x * (point.y)*(point.y);
    mat4 randomTiltMatrixX = mat4( 1, 0,       0,      0,
                                   0, cos(a), -sin(a), 0,
                                   0, sin(a),  cos(a), 0,
                                   0, 0,       0,      1
                                 );
    a = randomTiltAngle.y * (point.y)*(point.y);
    mat4 randomTiltMatrixZ = mat4( cos(a), -sin(a), 0, 0,
                                   sin(a),  cos(a), 0, 0,
                                   0,       0,      1, 0,
                                   0,       0,      0, 1
                                 );
    mat4 rotation = mat4( cos( rotationAngle ), 0,  sin( rotationAngle ), 0,
                          0,                    1,  0,                    0,
                         -sin( rotationAngle ), 0,  cos( rotationAngle ), 0,
                          0,                    0,  0,                    1
                        );

    points = point.xy;
    points.x += 0.5;
    points.y = 1-points.y;
    mat4 scaleMatrix = mat4(1.0);
    scaleMatrix[0][0] = 0.012*size;
    scaleMatrix[1][1] = 0.1*size;
    mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = position.x*2-1;
    positionMatrix[3][1] = position.y;
    positionMatrix[3][2] = position.z*2-1;
    
    norm = vec3(tiltMatrix * randomTiltMatrixX * randomTiltMatrixZ * rotation * vec4(norm, 1));
    point_world = vec3(positionMatrix * tiltMatrix * randomTiltMatrixX * randomTiltMatrixZ * rotation * scaleMatrix * point);
	gl_Position = camera * positionMatrix * tiltMatrix * randomTiltMatrixX * randomTiltMatrixZ * rotation * scaleMatrix * point;
}
