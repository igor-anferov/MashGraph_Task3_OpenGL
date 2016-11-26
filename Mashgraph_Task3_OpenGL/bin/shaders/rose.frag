#version 330 core

in vec2 TexCoords;
in vec3 norm;
in vec3 point_world;

out vec4 color;

uniform sampler2D texture_diffuse1;
uniform vec3 source_coord;
uniform vec3 cameraPos;


void main() {
    vec3 ambient_light = 0.2 * vec3(1,1,1);
    
    vec3 source_dir = normalize(source_coord - point_world);
    vec3 diffuse_light = 0.8 * max(dot(norm, source_dir), 0) * vec3(1,1,1);
    
    vec3 camera_dir = normalize(cameraPos - point_world);
    vec3 reflectDir = reflect(-source_dir, norm);
    vec3 specular_light = 0.1 * pow(max(dot(camera_dir, reflectDir), 0), 8) * vec3(1,1,1);
    
    vec3 light = ambient_light + diffuse_light + specular_light;
    
    color = vec4(texture(texture_diffuse1, TexCoords));
    if (color.a < 0.8) {
        discard;
    }
    color *= vec4(light,1) * max(0,point_world.y*10);
}
