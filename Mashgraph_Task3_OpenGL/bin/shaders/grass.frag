#version 330

in vec2 points;
in vec3 norm;
in vec3 point_world;
out vec4 outColor;

uniform sampler2D ourTexture;
uniform vec3 source_coord;
uniform vec3 cameraPos;

void main() {
    vec3 ambient_light = 0.1 * vec3(1,1,1);

    vec3 source_dir = normalize(source_coord - point_world);
    vec3 diffuse_light = 0.4 * abs(dot(norm, source_dir)) * vec3(1,1,1);
    
    vec3 camera_dir = normalize(cameraPos - point_world);
    vec3 reflectDir = reflect(-source_dir, norm);
    vec3 specular_light = 0.5 * pow(abs(dot(camera_dir, reflectDir)), 8) * vec3(1,1,1);

    vec3 light = ambient_light + diffuse_light + specular_light;
    
    outColor = texture(ourTexture, points);
    if(outColor.a < 0.8)
        discard;
    outColor *= vec4(light,1);
}
