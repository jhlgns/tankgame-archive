#version 330 core

uniform sampler2D Texture;

in vec2 TexCoords;
in vec4 Color;

out vec4 FragColor;

void main() {
    FragColor = Color * texture2D(Texture, TexCoords);
}
