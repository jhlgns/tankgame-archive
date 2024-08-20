#version 330 core

out vec2 TexCoords;
out vec4 Color;

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec4 aColor;

uniform mat4 Camera;

void main() {
    gl_Position = Camera * vec4(aPosition, 0.0, 1.0);
    TexCoords   = aTexCoords;
    Color       = aColor;
}
