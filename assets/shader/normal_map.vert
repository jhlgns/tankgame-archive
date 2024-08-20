#version 430 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec4 aColor;

const vec3 aNormal    = vec3(0.0, 0.0, 1.0);
const vec3 aTangent   = vec3(1.0, 0.0, 0.0);
const vec3 aBitangent = vec3(0.0, 1.0, 0.0);

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec4 Color;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} VsOut;

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

layout(std430, binding = 0) buffer ssbo_model
{
    mat4 Model[];
};

uniform mat4   Camera;
uniform SLight Light;
uniform vec3   ViewPos;

void main()
{
    VsOut.FragPos = vec3((Model[gl_InstanceID] * vec4(aPos, 0.0, 1.0)).xyz);
    VsOut.TexCoords = aTexCoords;
    VsOut.Color = aColor;
    
    mat3 NormalMatrix = transpose(inverse(mat3(Model[gl_InstanceID])));
    vec3 T = normalize(NormalMatrix * aTangent);
    vec3 N = normalize(NormalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));    
    VsOut.TangentLightPos = TBN * Light.Position;
    VsOut.TangentViewPos  = TBN * ViewPos;
    VsOut.TangentFragPos  = TBN * VsOut.FragPos;
        
    gl_Position = Camera * Model[gl_InstanceID] * vec4(aPos, 0.0, 1.0);
}
