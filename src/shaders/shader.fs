#version 450 core

out vec4 FragColor;
in vec3 FragPos;
in vec2 TexCoords;

uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoords);
}
