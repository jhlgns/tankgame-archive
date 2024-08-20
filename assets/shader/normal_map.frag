#version 330 core
#extension GL_ARB_separate_shader_objects : enable

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec4 Color;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} FsIn;

struct SLight {
  vec3 Position;

  vec3 Ambient;
  vec3 Diffuse;
  vec3 Specular;

  // Falloff function
  float Constant;
  float Linear;
  float Quadratic;
};

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalTexture;
uniform SLight    Light;

void main()
{
    //fragColor = FsIn.Color * texture(diffuse_texture, FsIn.TexCoords);
    //return;

    // Obtain normal from normal map in range [0,1] and convert
    vec3 Normal = texture(NormalTexture, FsIn.TexCoords).rgb;
    Normal = normalize(Normal * 2.0 - 1.0);
   
    // Get diffuse color
    vec3 Color = texture(DiffuseTexture, FsIn.TexCoords).rgb;

    // ambient
    vec3 Ambient = Light.Ambient * Color;

    // Diffuse
    vec3 LightDir = normalize(FsIn.TangentLightPos - FsIn.TangentFragPos);
    float Diff = max(dot(LightDir, Normal), 0.0);
    vec3 Diffuse = Light.Diffuse * Diff * Color;

    // Specular
    vec3 ViewDir = normalize(FsIn.TangentViewPos - FsIn.TangentFragPos);
    vec3 ReflectDir = reflect(-LightDir, Normal);
    vec3 HalfwayDir = normalize(LightDir + ViewDir);  
    float Spec = pow(max(dot(Normal, HalfwayDir), 0.0), 16.0);
    vec3 Specular = vec3(0.2) * Spec;

    // Falloff
    float Distance = length(Light.Position - FsIn.FragPos);
    float Attenuation = 1.0 / (Light.Constant + Light.Linear * Distance + Light.Quadratic * Distance * Distance);
    Ambient  *= Attenuation;
    Specular *= Attenuation;
    Diffuse  *= Attenuation;

    //fragColor = vec4(ambient + diffuse + spec, 1.0);
    vec3 Result = Ambient + Diffuse;
    Color = Result;

    /*if (Diff > 0.95) {
        Color = Result * 1.2;
    } else if (Diff > 0.7) {
        Color = Result;
    else if (Diff > 0.5) {
        Color = Result * 0.8;
    } else if (Diff > 0.25) {
        Color = Result * 0.6;
    } else {
        Color = Result * 0.4;
    }*/

    FragColor = FsIn.Color * vec4(Color, texture(DiffuseTexture, FsIn.TexCoords).a);
}
