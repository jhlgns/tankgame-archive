#version 330 core

in vec2 TexCoords;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D Texture;

void main() {
    float Red = texture2D(Texture, TexCoords).r;
    FragColor = vec4(1.0, 1.0, 1.0, Red) * Color;
}
