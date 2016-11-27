#version 330

in float instID;
in vec2 points;
in vec3 norm;
in vec3 point_world;
out vec4 outColor;

uniform sampler2D text1;
uniform sampler2D text2;
uniform sampler2D text3;
uniform vec3 source_coord;
uniform vec3 cameraPos;

void main() {
    vec3 ambient_light = 0.2 * vec3(1,1,1);
    
    vec3 source_dir = normalize(source_coord - point_world);
    vec3 diffuse_light = 0.5 * abs(dot(norm, source_dir)) * vec3(1,1,1);
    
    vec3 camera_dir = normalize(cameraPos - point_world);
    vec3 reflectDir = reflect(-source_dir, norm);
    vec3 specular_light = 0*0.2 * pow(abs(dot(camera_dir, reflectDir)), 8) * vec3(1,1,1);
    
    vec3 light = ambient_light + diffuse_light + specular_light;
    
    if (int(instID)==0)
        outColor = texture(text1, points);
    else
        if (int(instID)==1)
            outColor = texture(text2, points);
        else
            if (int(instID)==2)
                outColor = texture(text3, points);
            else
                outColor = vec4(1);
    if(outColor.a < 0.9)
        discard;
    outColor *= light.x;
}
