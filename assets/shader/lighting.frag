#version 330 core

in vec2 TexCoords;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D BaseTexture;
uniform sampler2D LightTexture;

void main() {
    FragColor = Color * texture2D(BaseTexture, TexCoords) * texture2D(LightTexture, TexCoords);
}
